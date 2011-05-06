namespace sys {
  template <typename T>
  class List {
    T* head_;
    T* tail_;
    int count_;

  public:
    void init() {
      head_ = 0;
      tail_ = 0;
      count_ = 0;
    }

    T* head() {
      return head_;
    }

    T* tail() {
      return tail_;
    }

    int count() {
      return count_;
    }

    void unlink(T* elem) {
      count_--;

      elem->list = 0;

      if(elem->next) {
        elem->next->prev = elem->prev;
      }

      if(elem->prev) {
        elem->prev->next = elem->next;
      }

      if(elem == head_) {
        head_ = elem->next;
      }

      if(elem == tail_) {
        tail_ = elem->prev;
      }
    }

    void append(T* elem) {
      elem->list = this;
      count_++;

      tail_->next = elem;
      elem->prev = tail_;
      elem->next = 0;

      tail_ = elem;
      if(!head_) head_ = tail_;
    }

    void prepend(T* elem) {
      elem->list = this;
      count_++;

      head_->prev = elem;
      elem->next = head_;
      elem->prev = 0;

      head_ = elem;
      if(!tail_) tail_ = head_;
    }

  };
};
