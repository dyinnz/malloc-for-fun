
#pragma once

#include <cassert>
#include <iostream>

template<class T>
class List {
 public:
  List() {
    dummy_head()->set_prev(dummy_head());
    dummy_head()->set_next(dummy_head());
  }

  void push(T *node) {
    assert(nullptr != node);
    /* before push:
     * head --- third (may be head)
     *
     * after push:
     * head --- node --- third (may be head)
     */
    T *third = dummy_head()->next();
    third->set_prev(node);

    node->set_next(third);
    node->set_prev(dummy_head());

    dummy_head()->set_next(node);

    size_ += 1;
  }

  T *pop() {
    if (empty()) {
      // TODO: shoud not be here
      assert(false);

      /* self loop:
      */
      return nullptr;

    } else {
      /* before pop:
       * head --- node --- third (may be head)
       *
       * after pop:
       * head --- third (may be head)
       */
      T *node = dummy_head()->next();
      T *third = node->next();
      dummy_head()->set_next(third);
      third->set_prev(dummy_head());

      size_ -= 1;
      return node;
    }
  }

  void erase(T *node) {
    assert(nullptr != node);
    /*
     * prev --- node --- next
     */
    T *prev = node->prev();
    T *next = node->next();
    prev->set_next(next);
    next->set_prev(prev);

    size_ -= 1;
  }

  T *first() {
    return dummy_head()->next();
  }

  T *end() {
    return dummy_head();
  }

  size_t size() const {
    return size_;
  }

  bool empty() const {
    return 0 == size();
  }

  void print() const {
    std::cout << "List: " << this << std::endl;
    for (T *node = dummy_head()->next(); node != dummy_head(); node = node->next()) {
      std::cout
          << " prev: " << node->prev()
          << " curr: " << node
          << " next: " << node->next() << std::endl;
    }
  }

 private:
  const T *dummy_head() const {
    return &head_data_;
  }

  T *dummy_head() {
    return &head_data_;
  }

 private:
  size_t size_{0};
  T head_data_;
};
