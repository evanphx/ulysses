#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include "kheap.hpp"

namespace sys {
  template <typename Key, typename Value, typename Operations>
  class HashTable {
    struct Entry {
      Key key;
      Value value;

      Entry* next;

      Entry(Key k, Value v)
        : key(k)
        , value(v)
        , next(0)
      {}
    };

    Entry** values_;
    u32 bins_;
    u32 entries_;

  public:
    const static u32 MinSize = 16;

    HashTable()
      : bins_(MinSize)
      , entries_(0)
    {
      values_ = (Entry**)kmalloc(bins_ * sizeof(Entry*));
      for(u32 i = 0; i < bins_; i++) {
        values_[i] = 0;
      }
    }

    u32 find_bin(u32 hash, u32 total) {
      return hash & (total - 1);
    }

    bool max_density_p() {
      return entries_ >= ((bins_ * 3) / 4);
    }

    bool min_density_p() {
      return entries_ < ((bins_ * 3) / 10);
    }

    void redistribute(u32 size) {
      Entry** new_values = (Entry**)kmalloc(size * sizeof(Entry*));
      for(u32 i = 0; i < size; i++) {
        new_values[i] = 0;
      }

      for(u32 i = 0; i < bins_; i++) {
        Entry* entry = values_[i];

        while(entry) {
          Entry* link = entry->next;
          entry->next = 0;

          u32 hash = Operations::compute_hash(entry->key);
          u32 bin = find_bin(hash, size);

          Entry* slot = new_values[bin];

          if(slot) {
            slot->next = entry;
          } else {
            new_values[bin] = entry;
          }

          entry = link;
        }
      }

      kfree(values_);

      values_ = new_values;
      bins_ = size;
    }

    void store(Key key, Value value) {
      if(max_density_p()) {
        redistribute(bins_ << 1);
      }

      u32 hash = Operations::compute_hash(key);
      u32 bin = find_bin(hash, bins_);

      Entry* last = 0;
      Entry* entry = values_[bin];

      while(entry) {
        if(Operations::compare_keys(entry->key, key)) {
          entry->value = value;
          return;
        }

        last = entry;
        entry = entry->next;
      }

      Entry* new_entry = new(kheap) Entry(key, value);

      if(last) {
        last->next = new_entry;
      } else {
        values_[bin] = new_entry;
      }

      entries_++;
    }

    bool fetch(Key key, Value* val) {
      u32 hash = Operations::compute_hash(key);
      u32 bin = find_bin(hash, bins_);

      Entry* entry = values_[bin];

      while(entry) {
        if(Operations::compare_keys(entry->key, key)) {
          *val = entry->value;
          return true;
        }

        entry = entry->next;
      }

      return false;
    }

    bool remove(Key key) {
      if(min_density_p()) {
        redistribute(bins_ >> 1);
      }

      u32 hash = Operations::compute_hash(key);
      u32 bin = find_bin(hash, bins_);

      Entry* last = 0;
      Entry* entry = values_[bin];

      while(entry) {
        if(Operations::compare_keys(entry->key, key)) {
          if(last) {
            last->next = entry->next;
          } else {
            values_[bin] = entry->next;
          }

          entries_--;
          kfree(entry);

          return true;
        }

        last = entry;
        entry = entry->next;
      }

      return false;
    }

    class Iterator {
      HashTable& tbl_;
      u32 bin_;
      Entry* entry_;

    public:
      Iterator(HashTable& tbl)
        : tbl_(tbl)
        , bin_(0)
        , entry_(0)
      {}

      Entry* next() {
        if(entry_) {
          if(entry_->next) {
            entry_ = entry_->next;
            return entry_;
          } else {
            bin_++;
          }
        }

        while(bin_ < tbl_.bins_) {
          Entry* e = tbl_.values_[bin_];
          if(e) {
            entry_ = e;
            return e;
          } else {
            bin_++;
          }
        }

        return 0;
      }
    };

    Iterator iterator() {
      return Iterator(*this);
    }

  };

  template <typename K>
  struct IdentityHashOperations {
    static u32 compute_hash(K k) {
      // See http://burtleburtle.net/bob/hash/integer.html
      register u32 a = (u32)k;
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }

    static bool compare_keys(K a, K b) {
      return ((u32)a) == ((u32)b);
    }
  };

  template <typename K, typename V>
    class IdentityHash : public HashTable<K, V, IdentityHashOperations<K> > {};

  template <typename K>
  struct OOHashOperations {
    static u32 compute_hash(K k) {
      // See http://burtleburtle.net/bob/hash/integer.html
      register u32 a = k.hash();
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }

    static bool compare_keys(K a, K b) {
      return a == b;
    }
  };

  template <typename K, typename V>
    class OOHash : public HashTable<K, V, OOHashOperations<K> > {};
}

#endif
