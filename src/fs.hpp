// fs.hpp -- Defines the interface for and structures relating to the virtual file system.
//         Written for JamesM's kernel development tutorials.

#ifndef FS_H
#define FS_H

#include "common.hpp"

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
    u32 impl;        // An implementation-defined number.

    Node *ptr; // Used by mountpoints and symlinks.

    virtual u32 read(u32 offset, u32 size, u8* buffer) { return 0; }
    virtual u32 write(u32 offset, u32 size, u8* buffer) { return 0; }
    virtual void open() { return; }
    virtual void close() { return; }
    virtual struct dirent* readdir(u32 index) { return 0; }
    virtual Node* finddir(const char* name) { return 0; }
};

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
