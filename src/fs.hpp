// fs.hpp -- Defines the interface for and structures relating to the virtual file system.
//         Written for JamesM's kernel development tutorials.

#ifndef FS_H
#define FS_H

#include "common.hpp"
#include "string.hpp"
#include "block.hpp"
#include "hash_table.hpp"

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 // Is the file an active mountpoint?

struct dirent {
  char name[128]; // Filename.
  u32 ino;     // Inode number. Required by POSIX.
};

namespace fs {

  struct Node {
    char name[128];     // The filename.
    u32 mask;        // The permissions mask.
    u32 uid;         // The owning user.
    u32 gid;         // The owning group.
    u32 flags;       // Includes the node type. See #defines above.
    u32 inode;       // This is device-specific - provides a way for a filesystem to identify files.
    u32 length;      // Size of the file, in bytes.

    Node* delegate; // Used by mountpoints and symlinks.

    virtual u32 read(u32 offset, u32 size, u8* buffer) { return 0; }
    virtual u32 write(u32 offset, u32 size, u8* buffer) { return 0; }
    virtual void open() { return; }
    virtual void close() { return; }
    virtual struct dirent* readdir(u32 index) { return 0; }
    virtual Node* finddir(const char* name, int len) { return 0; }
  };

  // An open file used by a process.
  class File {
    Node* node_;
    u32 offset_;

  public:
    File(Node* node);
    s32 read(u8* buffer, u32 size);
    s32 write(u8* buffer, u32 size);
    void seek(int pos, int whence);
  };

  class Registry;

  class RegisteredFS {
    RegisteredFS* next_;
    sys::FixedString<16> name_;

  public:

    RegisteredFS(const char* name)
      : next_(0)
      , name_(name)
    {}

    sys::FixedString<16>& name() {
      return name_;
    }

    virtual fs::Node* load(block::Device* dev) = 0;

    friend class Registry;
  };

  class Registry {
    RegisteredFS* head_;

  public:

    void init() {
      head_ = 0;
    }

    void add_fs(RegisteredFS* fs);
    RegisteredFS* find(const char* name);
  };

  extern Registry registry;

  fs::Node* lookup(const char* name, int len, fs::Node* dir);
  int mount(const char* path, const char* type, const char* dev_name);
}

extern fs::Node *fs_root; // The root of the filesystem.

// Standard read/write/open/close functions. Note that these are all suffixed with
// _fs to distinguish them from the read/write/open/close which deal with file descriptors
// , not file nodes.
u32 read_fs(fs::Node *node, u32 offset, u32 size, u8int *buffer);
u32 write_fs(fs::Node *node, u32 offset, u32 size, u8int *buffer);
void open_fs(fs::Node *node, u8int read, u8int write);
void close_fs(fs::Node *node);
struct dirent *readdir_fs(fs::Node *node, u32 index);
fs::Node *finddir_fs(fs::Node *node, char *name);

#endif
