#ifndef FS_DEVFS_HPP
#define FS_DEVFS_HPP

#include "common.hpp"
#include "fs.hpp"
#include "block.hpp"

namespace devfs {
  struct Node : public fs::Node {
    Node* next;

    block::Device* block_dev;

    virtual u32 read(u32 offset, u32 size, u8* buffer);
    virtual u32 write(u32 offset, u32 size, u8* buffer);
    virtual void open();
    virtual void close();
    virtual struct dirent* readdir(u32 index);
    virtual Node* finddir(const char* name, int len);
  };

  class DevFS {
    Node* head_;
    Node* root_;

  public:

    Node* head() {
      return head_;
    }

    Node* root() {
      return root_;
    }

    void init();
    Node* make_node(block::Device* dev, const char* name);
    void add_block_device(block::Device* dev, const char* name);
  };

  extern DevFS main;
};

#endif
