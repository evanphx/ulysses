// fs.hpp -- Defines the interface for and structures relating to the virtual file system.
//         Written for JamesM's kernel development tutorials.

#ifndef FS_H
#define FS_H

#include "common.hpp"
#include "string.hpp"
#include "block.hpp"
#include "hash_table.hpp"

#include "inttypes.h"

#include <stdarg.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 // Is the file an active mountpoint?

#define MAXNAMLEN 255

struct dirent {
  ino_t d_ino;
  off_t d_off;
  unsigned short int d_reclen;
  unsigned char d_type;
  char d_name[];
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

    Node()
      : mask(0)
      , uid(0)
      , gid(0)
      , flags(0)
      , inode(0)
      , length(0)
      , delegate(0)
    {
      name[0] = 0;
    }

    virtual u32 read(u32 offset, u32 size, u8* buffer) { return 0; }
    virtual u32 write(u32 offset, u32 size, u8* buffer) { return 0; }
    virtual void open() { return; }
    virtual int close() { return 0; }
    virtual struct dirent* readdir(u32 index) { return 0; }
    virtual Node* finddir(const char* name, int len) { return 0; }
    virtual Node* create_file(sys::String& str) { return 0; }
    virtual int get_entries(int offset, void* dp, int count) { return -1; }
    virtual int ioctl(unsigned long req, va_list args) { return -1; }
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
    int get_entries(void* dp, int count);
    int ioctl(unsigned long req, ...);
    int close();
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
