#include "elf.hpp"
#include "kheap.hpp"
#include "console.hpp"
#include "fs.hpp"
#include "paging.hpp"
#include "cpu.hpp"
#include "task.hpp"

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

  static u32 count_table(const char** tbl, u32* total) {
    u32 count = 0;
    u32 tot = 0;

    while(*tbl) {
      tot++;
      count += sizeof(char*); // the pointer itself
      count += strlen(*tbl); // the data at the pointer
      count += 1; // a null
      tbl++;
    }

    count += sizeof(char*); // for the null at the end

    *total = tot;
    return count;
  }

  bool load_node(Request& req) {
    if(req.node->length < sizeof(Header)) {
      console.printf("Invalid elf header.\n");
      return false;
    }

    u8* buffer = (u8*)kmalloc(sizeof(Header));
    req.node->read(0, sizeof(Header), buffer);

    Header* hdr = (Header*)buffer;

    if(!hdr->validate()) {
      console.printf("Invalid elf header.\n");
      kfree(buffer);
      return false;
    }

    if(hdr->e_phnum > 0) {
      ProgramHeader* first_ph = hdr->load_ph(req.node);
      ProgramHeader* ph = first_ph;

      for(int i = 0; i < hdr->e_phnum; i++) {
        if(ph->load_p()) {
          scheduler.current->add_mmap(req.node, ph->p_offset, ph->p_filesz,
                                      ph->p_vaddr, ph->p_memsz,
                                      ph->mmap_flags());
        }
        ph++;
      }

      kfree(first_ph);
    }

    u32 argc, envc;

    u32 arg_bytes = count_table(req.argv, &argc);
    u32 env_bytes = count_table(req.env,  &envc);

    u32 header_bytes = arg_bytes + env_bytes + sizeof(u32)
                     + (sizeof(char**) * 2);

    header_bytes = align(header_bytes, 16);

    u32 pages = cpu::page_align(header_bytes) / cpu::cPageSize;

    // Allocate frames for the memory and map it.
    for(u32 i = 1; i <= pages; i++) {
      u32 addr = KERNEL_VIRTUAL_BASE - (i * cpu::cPageSize);
      x86::Page* p = vmem.get_current_page(addr, 1);
      vmem.alloc_user_frame(p, true);
    }

    u32 data = KERNEL_VIRTUAL_BASE - arg_bytes - env_bytes;

    // Copy env
    u32 bytes_only = env_bytes - (sizeof(char*) * (argc + 1));

    u8* bytes = (u8*)data;

    u8** env_tbl = (u8**)(bytes + bytes_only);

    for(u32 i = 0; i < envc; i++) {
      const char* str = req.env[i];
      int str_len = strlen(str) + 1;  // +1 for the NULL too
      memcpy(bytes, (const u8*)str, str_len);

      env_tbl[i] = bytes;
      bytes += str_len;
    }

    env_tbl[envc] = 0;

    data += env_bytes;

    // Copy args
    bytes_only = arg_bytes - (sizeof(char*) * (argc + 1));

    bytes = (u8*)data;

    u8** arg_tbl = (u8**)(bytes + bytes_only);

    for(u32 i = 0; i < argc; i++) {
      const char* str = req.argv[i];
      int str_len = strlen(str) + 1;  // +1 for the NULL too
      memcpy(bytes, (const u8*)str, str_len);

      arg_tbl[i] = bytes;
      bytes += str_len;
    }

    arg_tbl[argc] = 0;

    data += arg_bytes;

    req.new_esp = KERNEL_VIRTUAL_BASE - header_bytes;

    // now the stack args that the program will see
    u32 arg_start = req.new_esp;

    // No return value
    *((u32*)arg_start) = 0;
    arg_start += sizeof(void*);

    *((u32*)arg_start) = argc;
    arg_start += sizeof(u32*);

    *((u8***)arg_start) = arg_tbl;
    arg_start += sizeof(u32*);

    *((u8***)arg_start) = env_tbl;

    u32 stack_fin = KERNEL_VIRTUAL_BASE - USER_STACK_SIZE;

    scheduler.current->add_mmap(0, 0, 0, stack_fin, USER_STACK_SIZE,
                                MemoryMapping::eAll);

    req.target_ip = hdr->e_entry;

    return true;
  }

  bool Request::load_file() {
    node = fs_root->finddir(path, strlen(path));
    if(!node) return false;
    return true;
  }
}
