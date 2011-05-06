// initrd.h -- Defines the interface for and structures relating to the initial ramdisk.
//             Written for JamesM's kernel development tutorials.

#ifndef INITRD_H
#define INITRD_H

#include "common.h"
#include "fs.h"

namespace initrd {

  struct Header {
    u32 nfiles; // The number of files in the ramdisk.
  };

  struct FileHeader {
    u8 magic;     // Magic number, for error checking.
    s8 name[64];  // Filename.
    u32 offset;   // Offset in the initrd that the file starts.
    u32 length;   // Length of the file.
  };

  struct FS;

  struct Node : public fs::Node {
    FS* fs;

    u32 read(u32 offset, u32 size, u8* buffer);
    struct dirent* readdir(u32 index);
    fs::Node* finddir(const char* name);
  };

  struct FS {
    FileHeader* file_headers; // The list of file headers.
    Node* root;             // Our root directory node.
    Node* dev;              // We also add a directory node for /dev, so we can mount devfs later on.
    Node* root_nodes;              // List of file nodes.
    u32 nroot_nodes;                    // Number of file nodes.
  
    Node* init(u32 location);
  };

  extern FS fs;

// Initialises the initial ramdisk. It gets passed the address of the multiboot module,
// and returns a completed filesystem node.
}

#endif
