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

  };
}

#endif
