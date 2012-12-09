#ifndef OWN_HPP
#define OWN_HPP

template <class T>
class own {
  T ptr_;

public:
  own(T ptr)
    : ptr_(ptr)
  {}

  ~own() {
    kfree(ptr_);
  }

  T operator*() const {
    return ptr_;
  }

  T operator->() const {
    return ptr_;
  }

  operator T() const {
    return ptr_;
  }
};

#endif
