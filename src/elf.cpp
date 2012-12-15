#include "elf.hpp"
#include "kheap.hpp"
#include "console.hpp"
#include "fs.hpp"
#include "paging.hpp"
#include "cpu.hpp"
#include "thread.hpp"
#include "scheduler.hpp"

#include "own.hpp"

namespace elf {

  bool Header::validate() {
    if(e_ident[EI_MAG0] != ELFMAG0 ||
         e_ident[EI_MAG1] != ELFMAG1 ||
         e_ident[EI_MAG2] != ELFMAG2 ||
         e_ident[EI_MAG3] != ELFMAG3) return false;

    if(e_ident[EI_CLASS] != ELFCLASS32) return false;
    if(e_ident[EI_DATA] != ELFDATA2LSB) return false;
    if(e_ident[EI_VERSION] != 1) return false;

    if(e_type != ET_EXEC) return false;
    if(e_machine != MT_X86) return false;
    if(e_version != 1) return false;

    return true;
  }

  ProgramHeader* Header::load_ph(fs::Node* node) {
    u8* buffer = (u8*)kmalloc(ph_size());
    node->read(e_phoff, ph_size(), buffer);

    return (ProgramHeader*)buffer;
  }

  Section* Header::find_section(u8* buffer, const char* name) {
    Section* sections = (Section*)(buffer + e_shoff);

    console.printf("%d <=> %d\n", sizeof(Section), e_shentsize);

    ASSERT(sizeof(Section) == e_shentsize);

    Section* strtab = sections + e_shstrndx;

    char* strings = (char*)buffer + strtab->sh_offset;

    for(u16 i = 0; i < e_shnum; i++) {
      Section* sec = (Section*)sections;
      char* s = (char*)(strings + sec->sh_name);
      console.printf("Considering section '%s'\n", s);
      if(strcmp(s, name) == 0) return sec;
    }

    return 0;
  }

  TableInfo::TableInfo(const char** tbl)
    : table(tbl)
    , bytes(0)
  {
    u32 i = 0;

    while(true) {
      const char* str = tbl[i];
      if(!str) break;

      bytes += (strlen(str) + 1);  // +1 for the NULL too

      i++;
    }

    entries = i;
    table_size = sizeof(char*) * (entries + 1);
  }

  Loader::Loader(Request& req)
    : req_(req)
    , new_esp_(0)
    , target_ip_(0)
  {}

  Header* Loader::load_header() {
    if(req_.node->length < sizeof(Header)) {
      console.printf("Invalid elf header.\n");
      return 0;
    }

    u8* buffer = (u8*)kmalloc(sizeof(Header));
    req_.node->read(0, sizeof(Header), buffer);

    Header* hdr = (Header*)buffer;

    if(!hdr->validate()) {
      console.printf("Invalid elf header.\n");
      kfree(buffer);
      return 0;
    }

    return hdr;
  }

  void Loader::map_memory(Header* hdr, Process* proc) {
    own<ProgramHeader*> first_ph = hdr->load_ph(req_.node);
    ProgramHeader* ph = first_ph;

    for(int i = 0; i < hdr->e_phnum; i++) {
      if(ph->load_p()) {
        // console.printf("Mapping %p (%x) from %x\n", ph->p_vaddr, ph->p_memsz,
                       // ph->p_offset);

        proc->add_mmap(req_.node, ph->p_offset, ph->p_filesz,
                                    ph->p_vaddr, ph->p_memsz,
                                    ph->mmap_flags());
      }
      ph++;
    }
  }

  void Loader::allocate_pages_for_header(u32 bytes) {
    u32 top = stack_top();
    u32 pages = cpu::page_align(bytes) / cpu::cPageSize;

    // Allocate frames for the memory and map it.
    for(u32 i = 1; i <= pages; i++) {
      u32 addr = top - (i * cpu::cPageSize);
      x86::Page* p = vmem.get_current_page(addr, 1);
      vmem.alloc_user_frame(p, true);
    }
  }

  char** Loader::copy_string_table(u32 data_addr, u32 table_addr,
                                   TableInfo& tbl)
  {
    char** table = (char**)table_addr;
    char* pos = (char*)data_addr;

    u32 size = tbl.entries;

    for(u32 i = 0; i < size; i++) {
      const char* str = tbl.table[i];
      int str_len = strlen(str) + 1;  // +1 for the NULL too
      memcpy((u8*)pos, (const u8*)str, str_len);

      console.printf("[%d] => %p (%s)\n", i, pos, str);

      table[i] = pos; 

      pos += str_len;
    }

    table[size] = 0;

    return table;
  }

  /*
   *
   * This is the "traditional" layout that pretty much all crt's
   * expect. Rather than fight it, just lay our data out the same.
  Layout on stack (high addresses to low addresses)
  
  env_sn \
  ...    | data
  env_s2 |
  env_s1 /
  argv_sn \
  ...     | data
  argv_s2 |
  argv_s1 /
  0
  0  <- auxv
  ptr(env_s1) \
  ptr(env_s2) | pointer table
  ...         |
  ptr(env_sn) / <- environ
  ptr(argv_s1) \
  ptr(argv_s2) | pointer table
  ...          |
  ptr(argv_sn) / <- argv
  argc

  */

  void Loader::setup_args() {
    TableInfo env(req_.env);
    TableInfo argv(req_.argv);

    u32 auxv_size = sizeof(void*) * 2;

    u32 top = stack_top();

    u32 total_bytes = env.total_size() + argv.total_size() +
                       auxv_size + sizeof(u32);

    u32 header_bytes = align(total_bytes, 16);

    allocate_pages_for_header(header_bytes);

    new_esp_ = top - header_bytes;

    u32 argc_addr = new_esp_;
    u32 argv_table = argc_addr + sizeof(u32);
    u32 env_table = argv_table + argv.table_size;
    u32 auxv_table = env_table + env.table_size;
    u32 argv_data = auxv_table + auxv_size;
    u32 env_data = argv_data + argv.data_size();

    copy_string_table(env_data,  env_table,  env);
    copy_string_table(argv_data, argv_table, argv);

    u32* auxv = (u32*)auxv_table;
    auxv[0] = 0;
    auxv[1] = 0;

    u32* argc_p = (u32*)argc_addr;
    *argc_p = argv.entries;
  }

  bool Loader::load_into(Process* proc) {
    Header* hdr = load_header();
    if(!hdr) return false;

    if(hdr->e_phnum > 0) map_memory(hdr, proc);

    setup_args();

    u32 stack_fin = stack_top() - USER_STACK_SIZE;

    scheduler.process()->add_mmap(0, 0, 0, stack_fin, USER_STACK_SIZE,
                                MemoryMapping::eAll);

    target_ip_ = hdr->e_entry;

    return true;
  }

  bool Request::load_file() {
    node = fs_root->finddir(path, strlen(path));
    if(!node) return false;
    return true;
  }
}
