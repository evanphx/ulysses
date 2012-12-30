class AtomicInt {
  int value_;

  AtomicInt(int i=0)
    : value_(i)
  {}

  void set(int c) {
    old = value_;
    while(!__sync_bool_compare_and_swap(&value_, old, c)) {
      old = value_;
    }
  }

  void add(int c) {
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
