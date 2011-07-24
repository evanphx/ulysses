#include "process.hpp"

#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"
#include "fs.hpp"
#include "fs/devfs.hpp"

Process::Process(int pid)
  : pid_(pid)
  , break_mapping_(0)
{
  for(int i = 0; i < 16; i++) {
    fds_[i] = 0;
  }

  for(int i = 0; i < 16; i++) {
    channels_[i] = 0;
  }
}

void Process::add_mmap(fs::Node* node, u32 offset, u32 size, u32 addr,
                       u32 mem_size, int flags)
{
  MemoryMapping mapping(addr, mem_size, node, offset, size, flags);
  mmaps_.append(mapping);
}

int Process::open_file(const char* path, int mode) {
  fs::Node* node = fs::lookup(path+1, strlen(path)-1, fs_root);

  if(!node) return -1;

  // block::Device* dev = block::registry.get(1);
  // fs::Node* node = devfs::main.make_node(dev, path);
  int fd = find_fd();

  fds_[fd] = new(kheap) fs::File(node);

  return fd;
}

fs::File* Process::get_file(int fd) {
  if(fd < 0 || fd >= 16) return 0;
  return fds_[fd];
}

MemoryMapping* Process::find_mapping(u32 addr) {
  Process::MMapList::Iterator i = mmaps_.begin();

  while(i.more_p()) {
    MemoryMapping& mmap = i.advance();

    if(mmap.contains_p(addr)) return &mmap;
  }

  return 0;
}

u32 Process::change_heap(int bytes) {
  if(!break_mapping_) {
    int flags = MemoryMapping::eAll;
    u32 addr = 0x2000000;
    MemoryMapping mapping(addr, bytes, 0, 0, 0, flags);
    break_mapping_ = &mmaps_.append(mapping);
    return addr;
  }

  u32 ret = break_mapping_->end_address();
  break_mapping_->enlarge_mem_size(bytes);
  return ret;
}

