// put stuff in here

namespace sys {

  template <int sz>
    class FixedString {
      const static int capacity_ = sz;
      char data_[sz];
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
      }

      FixedString(const char* str) {
        set(str);
      }

      FixedString(const char* str, int size) {
        if(size > capacity_) size = capacity_;
        size_ = size;
        memcpy(data_, str, size);
      }

      FixedString& operator=(const char* str) {
        set(str);
      }

      const char* c_str() {
        return data_;
      }

      int size() {
        return size_;
      }
    };
};
