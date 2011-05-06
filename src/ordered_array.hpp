// ordered_array.hpp -- Interface for creating, inserting and deleting
//                    from ordered arrays.
//                    Written for JamesM's kernel development tutorials.

#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include "common.hpp"

template <typename T, typename sorter>
struct ordered_array {
  T* array;
  u32 size;
  u32 max_size;

  void destroy() {
    // kfree(array->array);
  }

  void insert(T item) {
    u32 iterator = 0;
    while(iterator < size && sorter::less_than(array[iterator], item)) {
      iterator++;
    }

    if(iterator == size) { // just add at the end of the array.
      array[size++] = item;
    } else {
      T tmp = array[iterator];
      array[iterator] = item;
      while(iterator < size) {
        iterator++;
        T tmp2 = array[iterator];
        array[iterator] = tmp;
        tmp = tmp2;
      }
      size++;
    }
  }

  T lookup(u32 i) {
    ASSERT(i < size);
    return array[i];
  }

  void remove(u32 i) {
    while(i < size) {
      array[i] = array[i+1];
      i++;
    }
    size--;
  }

  static bool less_than(T a, T b) {
    return a < b;
  }
};

extern "C" u32int kmalloc(u32int sz);

template <typename T, typename S>
ordered_array<T,S> create_ordered_array(u32 max_size) {
  ordered_array<T,S> to_ret;
  to_ret.array = (void**)kmalloc(max_size*sizeof(T));
  memset((u8int*)to_ret.array, 0, max_size*sizeof(T));
  to_ret.size = 0;
  to_ret.max_size = max_size;
  return to_ret;
}

template <typename T, typename S>
ordered_array<T,S> place_ordered_array(void *addr, u32 max_size) {
  ordered_array<T,S> to_ret;
  to_ret.array = (T*)addr;
  memset((u8int*)to_ret.array, 0, max_size*sizeof(T));
  to_ret.size = 0;
  to_ret.max_size = max_size;
  return to_ret;
}

#endif // ORDERED_ARRAY_H
