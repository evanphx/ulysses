#include "fs/tmpfs.hpp"
#include "kheap.hpp"

namespace tmpfs {
  void init() {
    RegisteredFS* fs = new(kheap) RegisteredFS("tmpfs");
    fs::registry.add_fs(fs);
  }

  fs::Node* RegisteredFS::load(block::Device* dev) {
    if(dev) return 0;

    FS* fs = new (kheap) FS();
    return fs->root();
  }

  FS::FS() {
    DirectoryNode* node = new(kheap) DirectoryNode(this);

    node->name[0] = '/';
    node->name[1] = 0;

    node->mask = 0;
    node->uid = 0;
    node->gid = 0;
    node->flags = FS_DIRECTORY;
    node->inode = 0;
    node->length = 0;
    node->delegate = 0;

    root_ = node;
  }

  struct dirent* DirectoryNode::readdir(u32 index) {
    return 0;
  }


  fs::Node* DirectoryNode::finddir(const char* name, int size) {
    sys::String str(name, size, false);

    Node* n;
    if(entries_.fetch(str, &n)) {
      return n;
    }

    return 0;
  }

  fs::Node* DirectoryNode::create_file(sys::String& name) {
    Node* n;

    if(entries_.fetch(name, &n)) {
      return n;
    }

    FileNode* file = new(kheap) FileNode(fs_);

    sys::String copy = name;

    entries_.store(copy, file);
    return file;
  }

  int DirectoryNode::get_entries(int seek, void* dp, int count) {
    u8* buf_pos = (u8*)dp;
    u8* buf_fin = buf_pos + count;

    int total = 0;
    int pos = 0;

    if(buf_pos > buf_fin) return 0;

    u32 ent_size = align(sizeof(dirent) + 2, 4);

    if(seek == 0) {
      if(buf_pos + ent_size > buf_fin) return 0;

      dirent* de = (dirent*)buf_pos;
      de->d_ino = 1;

      de->d_name[0] = '.';
      de->d_name[1] = 0;
      de->d_reclen = ent_size;

      buf_pos += de->d_reclen;
      total += de->d_reclen;
    }

    pos += ent_size;

    ent_size = align(sizeof(dirent) + 3, 4);

    if(seek <= pos) {
      if(buf_pos > buf_fin) return total;
      if(buf_pos + ent_size > buf_fin) return total;

      dirent* de = (dirent*)buf_pos;
      de->d_ino = 1;
      de->d_reclen = ent_size;

      de->d_name[0] = '.';
      de->d_name[1] = '.';
      de->d_name[2] = 0;

      buf_pos += de->d_reclen;
      total += de->d_reclen;
    }

    pos += ent_size;

    auto iter = entries_.iterator();

    while(buf_pos < buf_fin) {
      auto entry = iter.next();
      if(!entry) break;

      int sz = entry->key.size();
      if(sz > 254) sz = 254;

      ent_size = align(sizeof(dirent) + sz + 1, 4);

      if(seek > pos) {
        pos += ent_size;
        continue;
      }

      if(buf_pos + ent_size > buf_fin) return total;

      dirent* de = (dirent*)buf_pos;
      de->d_ino = 1;
      de->d_reclen = ent_size;

      memcpy((u8*)de->d_name, (u8*)entry->key.data(), sz);
      de->d_name[sz] = 0;

      buf_pos += de->d_reclen;
      total += de->d_reclen;
    }

    return total;
  }

  FileNode::FileNode(FS* fs)
    : Node(fs)
    , chunk_((u8*)kmalloc(cChunkSize))
  {}

  u32 FileNode::read(u32 offset, u32 size, u8* buffer) {
    ASSERT(offset + size < cChunkSize);
    memcpy(buffer, chunk_ + offset, size);
    return size;
  }

  u32 FileNode::write(u32 offset, u32 size, u8* buffer) {
    ASSERT(offset + size < cChunkSize);
    memcpy(chunk_ + offset, buffer, size);
    return size;
  }
}
