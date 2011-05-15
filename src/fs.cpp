// fs.c -- Defines the interface for and structures relating to the virtual file system.
//         Written for JamesM's kernel development tutorials.

#include "fs.hpp"
#include "console.hpp"
#include "fs/devfs.hpp"

fs::Node *fs_root = 0; // The root of the filesystem.

namespace fs {
  File::File(Node* node)
    : node_(node)
    , offset_(0)
  {}

  s32 File::read(u8* buffer, u32 size) {
    s32 count = node_->read(offset_, size, buffer);
    if(count < 0) return count;
    offset_ += count;

    return count;
  }

  Node* lookup(const char* name, int len, fs::Node* dir) {
    int sub_len = 0;
    while(sub_len < len) {
      if(name[sub_len] == '/') break;
      sub_len++;
    }

    if(dir->delegate) dir = dir->delegate;

    fs::Node* child = dir->finddir(name, sub_len);

    if(child) {
      if(sub_len == len) return child;
      return lookup(name + (sub_len + 1), len - sub_len - 1, child);
    }

    return 0;
  }

  int mount(const char* name, const char* type) {
    console.printf("mounting %s, type=%s\n", name, type);
    fs::Node* node = lookup(name+1, strlen(name)-1, fs_root);
    if(!node) return -1;

    node->delegate = devfs::main.root();

    return 0;
  }
}

u32 read_fs(fs::Node *node, u32 offset, u32 size, u8int *buffer) {
  return node->read(offset, size, buffer);
}

u32 write_fs(fs::Node *node, u32 offset, u32 size, u8int *buffer) {
  return node->write(offset, size, buffer);
}

void open_fs(fs::Node *node, u8int read, u8int write) {
  node->open();
}

void close_fs(fs::Node *node) {
  node->close();
}

struct dirent *readdir_fs(fs::Node *node, u32 index) {
  // Is the node a directory, and does it have a callback?
  if((node->flags&0x7) == FS_DIRECTORY) {
    return node->readdir(index);
  }

  return 0;
}

fs::Node *finddir_fs(fs::Node *node, char *name) {
  // Is the node a directory, and does it have a callback?
  if((node->flags&0x7) == FS_DIRECTORY) {
    return node->finddir(name, strlen(name));
  }

  return 0;
}
