#ifndef LIST_HPP
#define LIST_HPP

#include "kheap.hpp"

namespace sys {
  template <typename T>
  struct ListNode {
    T* next;
    T* prev;
  };

  template <typename T, int which=0>
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

      count_++;

      if(tail_) {
        ListNode<T>& tn = tail_->lists[which];
        tn.next = elem;
      }

      node.prev = tail_;
      node.next = 0;
      tail_ = elem;
      if(!head_) head_ = tail_;
    }

    void prepend(T* elem) {
      ListNode<T>& node = elem->lists[which];

      count_++;

      if(head_) {
        ListNode<T>& hn = head_->lists[which];
        hn.prev = elem;
      }

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

  template <typename T>
  class ExternalList {
    struct Node {
      T elem;
      Node* next;
      Node* prev;

      Node(T e)
        : elem(e)
        , next(0)
        , prev(0)
      {}
    };

    Node* head_;
    Node* tail_;
    int count_;

  public:
    ExternalList()
      : head_(0)
      , tail_(0)
      , count_(0)
    {}

    T& head() {
      return head_->elem;
    }

    T& tail() {
      return tail_->elem;
    }

    int count() {
      return count_;
    }

    Node* find_node(T& e) {
      Node* node = head_;

      while(node) {
        if(node->elem == e) return node;
        node = node->next;
      }

      return 0;
    }

    void remove(T& elem) {
      count_--;

      Node* node = find_node(elem);

      if(node->next) {
        node->next->prev = node->prev;
      }

      if(node->prev) {
        node->prev->next = node->next;
      }

      if(node == head_) {
        head_ = node->next;
      }

      if(node == tail_) {
        tail_ = node->prev;
      }
    }

    T& append(T elem) {
      Node* node = new(kheap) Node(elem);

      count_++;

      if(tail_) {
        tail_->next = node;
      }

      node->prev = tail_;
      node->next = 0;

      tail_ = node;
      if(!head_) head_ = tail_;

      return node->elem;
    }

    void prepend(T elem) {
      Node* node = new(kheap) Node(elem);

      count_++;

      if(head_) {
        head_->prev = elem;
      }

      node->next = head_;
      node->prev = 0;

      head_ = node;
      if(!tail_) tail_ = head_;
    }

    class Iterator {
      Node* node;

    public:

      Iterator(Node* head)
        : node(head)
      {}

      bool more_p() {
        return node != 0;
      }

      T& advance() {
        Node* n = node;
        node = node->next;
        return n->elem;
      }
    };

    Iterator begin() {
      return Iterator(head_);
    }

  };
};

#endif
