#ifndef STRING_HPP
#define STRING_HPP

#include "kheap.hpp"

namespace sys {

  template <int sz>
    class FixedString {
      const static int capacity_ = sz;
      char data_[sz+1];
      int size_;

    public:

      FixedString()
        : size_(0)
      {}

      void set(const char* str) {
        int i = 0;

        for(; i < capacity_ ; i++) {
          char c = str[i];
          if(!c) break;
          data_[i] = c;
        }

        size_ = i;
        data_[size_] = 0;
      }

      FixedString(const char* str) {
        set(str);
      }

      FixedString(const char* str, int size) {
        if(size > capacity_) size = capacity_;
        size_ = size;
        memcpy(data_, str, size);
        data_[size_] = 0;
      }

      FixedString& operator=(const char* str) {
        set(str);
        return *this;
      }

      bool operator==(const char* str) {
        return !strcmp(data_, str);
      }

      FixedString& operator<<(char c) {
        int idx = size_++;
        ASSERT(size_ <= capacity_);

        data_[idx] = c;
        data_[size_] = 0;
        return *this;
      }

      const char* c_str() {
        return data_;
      }

      int size() {
        return size_;
      }
    };

  class String {
  public:
    enum Flags {
      eOwn =    1,
      eMalloc = 2,
      eFreeable = eOwn | eMalloc
    };

    const static u32 cLocalBuffer = 15;

  private:
    int flags_;
    u32 size_;

    u8* data_;
    u8 local_[cLocalBuffer+1];

  public:
    void set(const char* str, u32 size=0, bool own=true) {
      if(!str) {
        size_ = 0;
        data_ = 0;
        flags_ = 0;
        return;
      }

      if(!size) {
        size = strlen(str);
      }

      size_ = size;

      if(own) {
        flags_ = 0;

        if(size_ > 0) {
          if(size_ < cLocalBuffer) {
            data_ = 0;
            memcpy(local_, (const u8*)str, size_);
            local_[size_] = 0;
          } else {
            data_ = (u8*)kmalloc(size_ + 1);
            memcpy(data_, (const u8*)str, size_);
            data_[size_] = 0;
            flags_ |= eMalloc;
          }

          flags_ |= eOwn;
        } else {
          data_ = 0;
        }
      } else {
        flags_ = eMalloc;
        data_ = (u8*)str;
      }
    }

    String(const char* str, bool own=true) {
      set(str, strlen(str), own);
    }

    String(const char* str, u32 size, bool own=true) {
      set(str, size, own);
    }

    void set(String& other) {
      flags_ = 0;
      size_ = other.size_;

      if(size_ < cLocalBuffer) {
        data_ = 0;
        memcpy(local_, other.data(), size_);
        local_[size_] = 0;
      } else {
        data_ = (u8*)kmalloc(size_ + 1);
        memcpy(data_, other.data(), size_);
        data_[size_] = 0;
        flags_ |= eMalloc;
      }

      flags_ |= eOwn;
    }

    String(String& other) {
      set(other);
    }

    String& operator=(String& other) {
      set(other);
      return *this;
    }

    ~String() {
      if((flags_ & eFreeable) == eFreeable) kfree(data_);
    }

    u32 size() {
      return size_;
    }

    u8* data() {
      if(flags_ & eMalloc) return data_;
      return local_;
    }

    bool operator==(String& str) {
      if(str.size_ != size_) return false;
      return !strncmp((char*)data(), (char*)str.data(), size_);
    }

    const static u32 FNVOffsetBasis = 2166136261UL;
    const static u32 FNVHashPrime = 16777619UL;

    static inline unsigned long update_hash(unsigned long hv,
                                            unsigned char byte)
    {
      return (hv ^ byte) * FNVHashPrime;
    }

    static inline unsigned long finish_hash(unsigned long hv) {
      return (hv >> ((sizeof(u32) * 8) - 1)) ^ (hv & ~0);
    }

    static u32 compute_hash(const u8* bp) {
      u32 hv;

      hv = FNVOffsetBasis;

      while(*bp) {
        hv = update_hash(hv, *bp++);
      }

      return finish_hash(hv);
    }

    static u32 compute_hash(const u8* bp, u32 sz) {
      u8* be = (u8*)bp + sz;
      u32 hv = FNVOffsetBasis;

      while(bp < be) {
        hv = update_hash(hv, *bp++);
      }

      return finish_hash(hv);
    }

    u32 hash() {
      return compute_hash(data(), size_);
    }

  };
};

#endif
