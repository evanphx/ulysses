#include "fs/tmpfs.hpp"

namespace tar {
  class Archive {
    u8* buffer_;
    u32 size_;

  public:
    Archive(u8* buf, u32 size);
    int load_into(tmpfs::DirectoryNode* top);
  };
}
