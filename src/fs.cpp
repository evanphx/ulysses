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

  s32 File::write(u8* buffer, u32 size) {
    s32 count = node_->write(offset_, size, buffer);
    if(count < 0) return count;
    offset_ += count;

    return count;
  }

  void File::seek(int pos, int whence) {
    switch(whence) {
    default:
    case 0: // SEEK_SET
      offset_ = (u32)pos;
      break;
    case 1: // SEEK_CUR
      offset_ = (u32)((int)offset_ + pos);
      break;
    case 2: // SEEK_END
      offset_ = (u32)((int)node_->length + pos);
      break;
    }
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

  int mount(const char* name, const char* type, const char* dev_name) {
    console.printf("mounting %s to %s, type=%s\n", dev_name, name, type);
    fs::Node* node = lookup(name+1, strlen(name)-1, fs_root);
    if(!node) return -1;

    RegisteredFS* fs = registry.find(type);
    if(!fs) {
      console.printf("Unknown fs type '%s' to mount\n", type);
      return -1;
    }

    // Find the block device from dev_name
    block::Device* dev;

    // If none, then pass none to the fs.
    if(!dev_name) {
      dev = 0;
    } else {
      fs::Node* node = lookup(dev_name+1, strlen(dev_name)-1, fs_root);
      if(!node) {
        console.printf("Couldn't find device '%s' to mount.\n", dev_name);
        return -1;
      }

      if(node->flags != FS_BLOCKDEVICE) {
        console.printf("%s isn't a block device.\n", dev_name);
        return -1;
      }

      dev = block::registry.get(node->inode);
      if(!dev) {
        console.printf("Block device '%s' isn't setup right.\n", dev_name);
        return -1;
      }
    }

    fs::Node* root = fs->load(dev);

    if(!root) {
      console.printf("Unable to mount %s as type %s (%x)\n", dev_name, type, dev);
      return -1;
    }

    node->delegate = root;

    return 0;
  }

  Registry registry;

  void Registry::add_fs(RegisteredFS* fs) {
    if(!head_) {
      head_ = fs;
    } else {
      head_->next_ = fs;
    }
  }

  RegisteredFS* Registry::find(const char* name) {
    RegisteredFS* node = head_;
    while(node) {
      if(node->name() == name) return node;
      node = node->next_;
    }

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
