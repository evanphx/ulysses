#include "devfs.hpp"
#include "kheap.hpp"

namespace devfs {
  DevFS main;

  fs::Node* RegisteredFS::load(block::Device* dev) {
    return main.root();
  }

  void DevFS::init() {
    head_ = 0;
    fs_ = new(kheap) RegisteredFS("devfs");

    fs::registry.add_fs(fs_);

    root_ = new(kheap) Node;
    root_->name[0] = '/';
    root_->name[1] = 0;

    root_->mask = 0;
    root_->uid = 0;
    root_->gid = 0;
    root_->flags = FS_DIRECTORY;
    root_->inode = 0;
    root_->length = 0;
    root_->delegate = 0;
    root_->next = 0;
  }

  void DevFS::add_block_device(block::Device* dev, const char* name) {
    BlockNode* node = new(kheap) BlockNode;

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

    if(!head_) {
      head_ = node;
    } else {
      head_->next = node;
    }
  }

  void DevFS::add_char_device(character::Device* dev, const char* name) {
    CharacterNode* node = new(kheap) CharacterNode;

    int size = strlen((char*)name);
    if(size > 127) size = 127;
    memcpy((u8*)node->name, (u8*)name, size);
    node->name[size] = 0;

    node->mask = 0;
    node->uid = 0;
    node->gid = 0;
    node->flags = FS_BLOCKDEVICE;
    node->char_dev = dev;
    node->inode = 0;
    node->length = 0;
    node->delegate = 0;
    node->next = 0;

    if(!head_) {
      head_ = node;
    } else {
      head_->next = node;
    }
  }

  u32 BlockNode::read(u32 offset, u32 size, u8* buffer) {
    return block_dev->read_bytes(offset, size, buffer);
  }

  u32 CharacterNode::read(u32 offset, u32 size, u8* buffer) {
    return char_dev->read_bytes(offset, size, buffer);
  }

  u32 CharacterNode::write(u32 offset, u32 size, u8* buffer) {
    return char_dev->write_bytes(offset, size, buffer);
  }

  Node* Node::finddir(const char* name, int len) {
    Node* node = main.head();

    while(node) {
      if(!strncmp(node->name, name, len)) return node;
      node = node->next;
    }

    return 0;
  }
}
