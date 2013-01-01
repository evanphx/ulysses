#ifndef ATOMIC_HPP
#define ATOMIC_HPP

template <typename T=int>
class AtomicInt {
  T value_;

public:
  AtomicInt(T i=0)
    : value_(i)
  {}

  T value() {
    return value_;
  }

  void set(T c) {
    T old = value_;
    while(!__sync_bool_compare_and_swap(&value_, old, c)) {
      old = value_;
    }
  }

  void add(T c) {
    __sync_fetch_and_add(&value_, c);
  }

  void inc() {
    add(1);
  }

  void sub(int c) {
    __sync_fetch_and_sub(&value_, c);
  }

  void dec() {
    sub(1);
  }
};

#endif
