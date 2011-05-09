namespace sys {
  template <typename T>
  struct ListNode {
    T* next;
    T* prev;
  };

  template <typename T, int which>
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

      ListNode<T>& node = elem->lists[which];

      if(node.next) {
        node.next->lists[which].prev = node.prev;
      }

      if(node.prev) {
        node.prev->lists[which].next = node.next;
      }

      if(elem == head_) {
        head_ = node.next;
      }

      if(elem == tail_) {
        tail_ = node.prev;
      }
    }

    void append(T* elem) {
      ListNode<T>& node = elem->lists[which];
      ListNode<T>& tn = tail_->lists[which];

      count_++;

      tn.next = elem;
      node.prev = tail_;
      node.next = 0;

      tail_ = elem;
      if(!head_) head_ = tail_;
    }

    void prepend(T* elem) {
      ListNode<T>& node = elem->lists[which];
      ListNode<T>& hn = head_->lists[which];

      count_++;

      hn.prev = elem;
      node.next = head_;
      node.prev = 0;

      head_ = elem;
      if(!tail_) tail_ = head_;
    }

    class Iterator {
      T* node;

    public:

      Iterator(T* head)
        : node(head)
      {}

      bool more_p() {
        return node != 0;
      }

      T* advance() {
        T* n = node;
        node = node->lists[which].next;
        return n;
      }
    };

    Iterator begin() {
      return Iterator(head_);
    }

  };
};
