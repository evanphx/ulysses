#include "devfs.hpp"
#include "kheap.hpp"

namespace devfs {
  DevFS main;

  void DevFS::init() {
    head_ = 0;

    root_ = new(kheap) Node;
    root_->name[0] = '/';
    root_->name[1] = 0;

    root_->mask = 0;
    root_->uid = 0;
    root_->gid = 0;
    root_->flags = FS_DIRECTORY;
    root_->block_dev = 0;
    root_->inode = 0;
    root_->length = 0;
    root_->delegate = 0;
    root_->next = 0;
  }

  Node* DevFS::make_node(block::Device* dev, const char* name) {
    Node* node = new(kheap) Node;

    int size = strlen((char*)name);
    if(size > 127) size = 127;
    memcpy((u8*)node->name, (u8*)name, size);
    node->name[size] = 0;

    node->mask = 0;
    node->uid = 0;
    node->gid = 0;
    node->flags = FS_BLOCKDEVICE;
    node->block_dev = dev;
    node->inode = dev->id();
    node->length = 0;
    node->delegate = 0;
    node->next = 0;

    return node;
  }

  void DevFS::add_block_device(block::Device* dev, const char* name) {
    Node* node = make_node(dev, name);

    if(!head_) {
      head_ = node;
    } else {
      head_->next = node;
    }
  }

  u32 Node::read(u32 offset, u32 size, u8* buffer) {
    return block_dev->read_bytes(offset, size, buffer);
  }

  Node* Node::finddir(const char* name, int len) {
    Node* node = main.head();

    while(node) {
      if(!strncmp(node->name, name, len)) return node;
      node = node->next;
    }

    return 0;
  }
  
  u32 Node::write(u32 offset, u32 size, u8* buffer) { return 0; }
  void Node::open() { return; }
  void Node::close() { return; }
  struct dirent* Node::readdir(u32 index) { return 0; }
}
