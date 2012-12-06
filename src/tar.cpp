#include "common.hpp"
#include "tar.hpp"
#include "console.hpp"
#include "fs/tmpfs.hpp"

#include "zlib.h"
#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"

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

  static void* my_zalloc(void* cookie, uInt items, uInt size) {
    return (void*)kmalloc(items * size);
  }

  static void my_zfree(void* cookie, void* addr) {
    kfree(addr);
  }

  int Archive::load_into(tmpfs::DirectoryNode* top) {
    int count = 0;
    int ret;

    z_stream strm;
    u8 out[1024];

    strm.zalloc = my_zalloc;
    strm.zfree = my_zfree;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, 15 + 32);

    ASSERT(ret == Z_OK);

    strm.avail_in = size_;
    strm.next_in = buffer_;

    while(true) {
      strm.avail_out = 512;
      strm.next_out = out;

      ret = inflate(&strm, Z_NO_FLUSH);
      ASSERT(ret == Z_OK);

      Entry* ent = (Entry*)out;

      if(ent->empty_p()) break;

      count++;

      // Now at the file data.
      //

      u32 bytes = ent->byte_size();

      sys::String name(ent->name);

      tmpfs::FileNode* node = top->create_file(name);
      strcpy(node->name, ent->name);
      node->length = bytes;

      u8* file = node->resize(bytes);

      u32 left = bytes;

      while(left > 0) {
        u32 req;
        if(left >= 1024) {
          req = 1024;
        } else {
          req = left;
        }

        strm.avail_out = req;
        strm.next_out = file;

        ret = inflate(&strm, Z_NO_FLUSH);
        int read = req - strm.avail_out;

        if(ret == Z_STREAM_END) break;
        ASSERT(ret == Z_OK);

        file += read;
        left -= read;
        if(strm.avail_out != 0) break;
      }

      strm.avail_out = bytes % 512;
      strm.next_out = out;

      ret = inflate(&strm, Z_NO_FLUSH);
      ASSERT(ret == Z_OK);
    }

    (void)inflateEnd(&strm);

    return count;
  }
}

extern "C" void show_int(unsigned int x) {
  console.printf("int => %d\n", x);
}
