#ifndef FS_DEVFS_HPP
#define FS_DEVFS_HPP

#include "common.hpp"
#include "fs.hpp"
#include "block.hpp"
#include "character.hpp"

namespace devfs {

  struct Node : public fs::Node {
    Node* next;
    Node* finddir(const char* name, int len);
  };

  struct BlockNode : public Node {
    block::Device* block_dev;

    u32 read(u32 offset, u32 size, u8* buffer);

    Node* finddir(const char* name, int len) {
      return 0;
    }
  };

  struct CharacterNode : public Node {
    character::Device* char_dev;

    u32 read(u32 offset, u32 size, u8* buffer);
    u32 write(u32 offset, u32 size, u8* buffer);

    Node* finddir(const char* name, int len) {
      return 0;
    }

    int ioctl(unsigned long req, va_list args) {
      return char_dev->ioctl(req, args);
    }
  };

  class RegisteredFS : public fs::RegisteredFS {
  public:
    RegisteredFS(const char* name)
      : fs::RegisteredFS(name)
    {}

    fs::Node* load(block::Device* dev);
  };

  class DevFS {
    Node* head_;
    Node* root_;

    RegisteredFS* fs_;

  public:

    Node* head() {
      return head_;
    }

    Node* root() {
      return root_;
    }

    void init();
    void add_block_device(block::Device* dev, const char* name);
    void add_char_device(character::Device* dev, const char* name);
  };


  extern DevFS main;
};

#endif
