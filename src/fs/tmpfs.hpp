#ifndef FS_TMPFS_HPP
#define FS_TMPFS_HPP

#include "common.hpp"
#include "fs.hpp"
#include "hash_table.hpp"

#include "string.hpp"

namespace tmpfs {
  class FS;

  class Node : public fs::Node {
  protected:
    FS* fs_;

  public:
    Node(FS* fs)
      : fs_(fs)
    {}
  };

  class FileNode : public Node {
    u32 size_;
    u8* chunk_;

  public:
    static const u32 cInitialChunkSize = 1024;

    FileNode(FS* fs);

    u32 read(u32 offset, u32 size, u8* buffer);
    u32 write(u32 offset, u32 size, u8* buffer);
    void import_raw(u8* buffer, u32 size);
    u8* resize(u32 size);
  };

  typedef sys::OOHash<sys::String, Node*> NodeHash;

  class DirectoryNode : public Node {
    NodeHash entries_;

  public:
    DirectoryNode(FS* fs)
      : Node(fs)
    {}

    struct dirent* readdir(u32 index);
    fs::Node* finddir(const char* name, int size);
    tmpfs::FileNode* create_file(sys::String& str);
    int get_entries(int pos, void* dp, int count);
  };

  class FS {
    DirectoryNode* root_;

  public:
    FS();

    fs::Node* root() {
      return root_;
    }

    DirectoryNode* specific_root() {
      return root_;
    }
  };

  class RegisteredFS : public fs::RegisteredFS {
  public:
    RegisteredFS(const char* name)
      : fs::RegisteredFS(name)
    {}

    fs::Node* load(block::Device* dev);
  };

  void init();
}

#endif
