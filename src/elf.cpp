#include "elf.hpp"
#include "kheap.hpp"
#include "console.hpp"
#include "fs.hpp"
#include "paging.hpp"
#include "cpu.hpp"

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

  u32 load_node(fs::Node* node) {
    if(node->length < sizeof(Header)) {
      console.printf("Invalid elf header.\n");
      return 0;
    }

    u8* buffer = (u8*)kmalloc(sizeof(Header));
    node->read(0, sizeof(Header), buffer);

    Header* hdr = (Header*)buffer;

    if(!hdr->validate()) {
      console.printf("Invalid elf header.\n");
      kfree(buffer);
      return 0;
    }

    if(hdr->e_phnum > 0) {
      ProgramHeader* first_ph = hdr->load_ph(node);
      ProgramHeader* ph = first_ph;

      for(int i = 0; i < hdr->e_phnum; i++) {
        if(ph->load_p()) {
          page* p = vmem.get_current_page(ph->p_vaddr, 1);
          vmem.alloc_frame(p, 0, 0);
          // console.printf("mapped %x to %x\n", ph->p_vaddr, p->frame);

          node->read(ph->p_offset, ph->p_filesz, (u8*)ph->p_vaddr); 
        }
        ph++;
      }

      kfree(first_ph);

    }

    return hdr->e_entry;
  }
}
