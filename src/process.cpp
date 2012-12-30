#include "process.hpp"

#include "paging.hpp"
#include "kheap.hpp"
#include "descriptor_tables.hpp"
#include "cpu.hpp"
#include "console.hpp"
#include "timer.hpp"
#include "fs.hpp"
#include "fs/devfs.hpp"

Process::Process(int pid, PosixSession& session)
  : pid_(pid)
  , session_(session)
  , break_mapping_(0)
  , thread_ids_(0)
  , next_mmap_start_(cDefaultMMapStart)
{
  for(int i = 0; i < 16; i++) {
    fds_[i] = 0;
  }

  for(int i = 0; i < 16; i++) {
    channels_[i] = 0;
  }

  threads_.init();
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

  node->open();

  // block::Device* dev = block::registry.get(1);
  // fs::Node* node = devfs::main.make_node(dev, path);
  int fd = find_fd();

  fds_[fd] = new(kheap) fs::File(node);

  return fd;
}

int Process::dup_fd(int fd) {
  fs::File* file = get_file(fd);
  if(!file) return -1;

  int new_fd = find_fd();

  fds_[new_fd] = file;

  return new_fd;
}

fs::File* Process::get_file(int fd) {
  if(fd < 0 || fd >= 16) return 0;
  return fds_[fd];
}

MemoryMapping* Process::find_mapping(u32 addr) {
  Process::MMapList::Iterator i = mmaps_.begin();

  while(i.more_p()) {
    MemoryMapping& mmap = i.advance();

    if(mmap.contains_p(addr)) {
      return &mmap;
    }
  }

  return 0;
}

void Process::print_mmaps() {
  auto i = mmaps_.begin();

  while(i.more_p()) {
    MemoryMapping& mmap = i.advance();

    console.printf("%p-%p (%d) %s\n",
                   mmap.page_start(), mmap.page_end(),
                   mmap.mem_size(),
                   mmap.writable_p() ? "read-write" : "read-only");
  }
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

u32 Process::new_mmap_region(u32 size) {
  u32 addr = next_mmap_start_;
  next_mmap_start_ += size;
  return addr;
}

void Process::position_brk(u32 fin) {
  MemoryMapping mapping(fin, 0, 0, 0, 0, MemoryMapping::eAll);
  break_mapping_ = &mmaps_.append(mapping);
}

u32 Process::set_brk(u32 target) {
  if(!break_mapping_) {
    int flags = MemoryMapping::eAll;
    u32 addr = cDefaultBreakStart;
    u32 bytes = 0;

    if(target == 0) {
      bytes = 0;
      target = addr;
    } else if(target < addr) {
      console.printf("ignoring request to reduce brk %p < %p\n", target, addr);
      return target;
    } else {
      bytes = (u32)target - addr;
    }

    MemoryMapping mapping(addr, bytes, 0, 0, 0, flags);
    break_mapping_ = &mmaps_.append(mapping);
    return target;
  }

  if(target == 0) {
    return cDefaultBreakStart;
  } else if(target < break_mapping_->end_address()) {
    console.printf("ignoring request to reduce brk %p < %p\n",
                   target, break_mapping_->end_address());
    return target;
  }

  u32 ret = break_mapping_->end_address();
  break_mapping_->enlarge_mem_size(target - ret);
  return target;

}

void Process::exit(int code) {
  alive_ = false;
  exit_code_ = code;

  auto i = threads_.begin();

  while(i.more_p()) {
    Thread* t = i.advance();
    ASSERT(t->process() == this);
    t->die();
  }
}

Thread* Process::new_thread(void* placed) {
  return new(placed) Thread(this, thread_ids_++);
}
