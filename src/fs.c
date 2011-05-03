// fs.c -- Defines the interface for and structures relating to the virtual file system.
//         Written for JamesM's kernel development tutorials.

#include "fs.h"

fs::Node *fs_root = 0; // The root of the filesystem.

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
    return node->finddir(name);
  }

  return 0;
}
