#ifndef STRING_HPP
#define STRING_HPP

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
};

#endif
