// initrd.c -- Defines the interface for and structures relating to the initial ramdisk.
//             Written for JamesM's kernel development tutorials.

#include "initrd.hpp"
#include "kheap.hpp"

struct dirent dirent;

namespace initrd {
  FS fs;

  u32 Node::read(u32 offset, u32 size, u8 *buffer) {
    FileHeader header = fs->file_headers[inode];
    if(offset > header.length) return 0;

    if(offset+size > header.length) {
      size = header.length-offset;
    }

    memcpy(buffer, (u8*)(header.offset+offset), size);
    return size;
  }

  struct dirent* Node::readdir(u32 index) {
    if(this == fs->root && index == 0) {
      strcpy(dirent.d_name, "dev");
      dirent.d_name[3] = 0;
      dirent.d_ino = 0;
      return &dirent;
    }

    if(index-1 >= fs->nroot_nodes) return 0;

    strcpy(dirent.d_name, fs->root_nodes[index-1].name);
    dirent.d_name[strlen(fs->root_nodes[index-1].name)] = 0;
    dirent.d_ino = fs->root_nodes[index-1].inode;
    return &dirent;
  }

  fs::Node* Node::finddir(const char *name, int len) {
    if(this == fs->root && len == 3 && !strncmp(name, "dev", len)) {
      return fs->dev;
    }

    if(this == fs->root && len == 4 && !strncmp(name, "data", len)) {
      return fs->data;
    }

    if(this == fs->root && len == 3 && !strncmp(name, "tmp", len)) {
      return fs->tmp;
    }

    for(u32 i = 0; i < fs->nroot_nodes; i++) {
      if(!strncmp(name, fs->root_nodes[i].name, len)) {
        return &fs->root_nodes[i];
      }
    }
    return 0;
  }

  initrd::Node* FS::init(u32 location) {
    initrd::Header *initrd_header;     // The header.

    // Initialise the main and file header pointers and populate the root directory.
    initrd_header = (initrd::Header *)location;
    file_headers = (FileHeader *)(location+sizeof(initrd::Header));

    // Initialise the root directory.
    root = knew<initrd::Node>();
    strcpy(root->name, "initrd");
    root->mask = root->uid = root->gid = 
      root->inode = root->length = 0;
    root->flags = FS_DIRECTORY;
    root->delegate = 0;
    root->fs = this;

    // Initialise the /dev directory (required!)
    dev = knew<initrd::Node>();
    strcpy(dev->name, "dev");
    dev->mask = dev->uid = dev->gid =
      dev->inode = dev->length = 0;
    dev->flags = FS_DIRECTORY;
    dev->delegate = 0;
    dev->fs = this;

    data = knew<initrd::Node>();
    strcpy(data->name, "data");
    data->mask = data->uid = data->gid =
      data->inode = data->length = 0;
    data->flags = FS_DIRECTORY;
    data->delegate = 0;
    data->fs = this;

    tmp = knew<initrd::Node>();
    strcpy(tmp->name, "tmp");
    tmp->mask = tmp->uid = tmp->gid =
      tmp->inode = tmp->length = 0;
    tmp->flags = FS_DIRECTORY;
    tmp->delegate = 0;
    tmp->fs = this;

    nroot_nodes = initrd_header->nfiles;
    root_nodes = knew_array<initrd::Node>(nroot_nodes);

    // For every file...
    for(u32 i = 0; i < initrd_header->nfiles; i++) {
      // Edit the file's header - currently it holds the file offset
      // relative to the start of the ramdisk. We want it relative to the start
      // of memory.
      file_headers[i].offset += location;

      initrd::Node* node = &root_nodes[i];

      node->fs = this;

      // Create a new file node.
      strcpy(node->name, (const char*)&file_headers[i].name);
      node->mask = node->uid = node->gid = 0;
      node->length = file_headers[i].length;
      node->inode = i;
      node->flags = FS_FILE;
    }
    return root;
  }

}
