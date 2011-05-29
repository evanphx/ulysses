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
    u8* chunk_;

  public:
    static const u32 cChunkSize = 1024;

    FileNode(FS* fs);

    u32 read(u32 offset, u32 size, u8* buffer);
    u32 write(u32 offset, u32 size, u8* buffer);
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
    fs::Node* create_file(sys::String& str);
    int get_entries(int pos, void* dp, int count);
  };

  class FS {
    DirectoryNode* root_;

  public:
    FS();

    fs::Node* root() {
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
