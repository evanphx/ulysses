#include "common.hpp"
#include "tar.hpp"
#include "console.hpp"
#include "fs/tmpfs.hpp"

namespace tar {

  u32 ato8(char* buf, int size) {
    int i = 0;
    while(buf[i] == '0') i++;

    u32 out = 0;

    // console.printf("ato8: %s\n", buf);

    while(i < size && buf[i]) {
      // console.printf("out: %d (%s, %d)\n", out, buf + i, i);
      out += (buf[i] - '0');
      if(i < size - 1) out *= 8;

      i++;
    }

    return out;
  }

  struct Entry {
    char name[100];     // ASCII + (Z unless filled)
    char mode[8];       // 0 padded, octal, null
    char uid[8];        // ditto
    char gid[8];        // ditto
    char size[12];      // 0 padded, octal, null
    char mtime[12];     // 0 padded, octal, null
    char checksum[8];   // 0 padded, octal, null, space
    char typeflag[1];   // file: "0"  dir: "5"
    char linkname[100]; // ASCII + (Z unless filled)
    char magic[6];      // "ustar\0"
    char version[2];    // "00"
    char uname[32];     // ASCIIZ
    char gname[32];     // ASCIIZ
    char devmajor[8];   // 0 padded, octal, null
    char devminor[8];   // o padded, octal, null
    char prefix[155];   // ASCII + (Z unless filled)

    bool directory_p() {
      return typeflag[0] == '5';
    }

    bool file_p() {
      return typeflag[0] == '0';
    }

    u32 byte_size() {
      return ato8(size, 11);
    }

    bool empty_p() {
      u8* b = (u8*)this;
      for(int i = 0; i < 512; i++) {
        if(b[i]) return false;
      }

      return true;
    }
  };

  Archive::Archive(u8* buf, u32 size)
    : buffer_(buf)
    , size_(size)
  {}

  int Archive::load_into(tmpfs::DirectoryNode* top) {
    int count = 0;
    u8* pos = buffer_;
    u8* fin = buffer_ + size_;

    while(pos < fin) {
      Entry* ent = (Entry*)pos;

      if(ent->empty_p()) break;

      // console.printf("tar: %s (%s, %d)\n", ent->name,
                     // ent->size, ent->byte_size());
      count++;
      pos += 512;

      // Now at the file data.
      //

      u32 bytes = ent->byte_size();

      sys::String name(ent->name);

      tmpfs::FileNode* node = top->create_file(name);

      node->import_raw(pos, bytes);
      strcpy(node->name, ent->name);
      node->length = bytes;

      console.printf("first 4 bytes: %x %x %x %x\n", pos[0], pos[1], pos[2], pos[3]);

      pos += align(bytes, 512);
    }

    return count;
  }
}
