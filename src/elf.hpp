#ifndef ELF_H
#define ELF_H

#include "common.hpp"
#include "fs.hpp"
#include "paging.hpp"

namespace elf {

  enum Ident {
    EI_MAG0 = 0,
    EI_MAG1 = 1,
    EI_MAG2 = 2,
    EI_MAG3 = 3,
    EI_CLASS =4,
    EI_DATA = 5,
    EI_VERSION = 6,
    EI_OSABI = 7,
    EI_ABIVERSION = 8,
    EI_PAD = 9,
    EI_NIDENT = 16
  };

  const static u8 ELFMAG0 = 0x7f; // e_ident[EI_MAG0]
  const static u8 ELFMAG1 = 'E';  // e_ident[EI_MAG1]
  const static u8 ELFMAG2 = 'L';  // e_ident[EI_MAG2]
  const static u8 ELFMAG3 = 'F';  // e_ident[EI_MAG3]

  enum IdentClass {
    ELFCLASSNONE = 0,
    ELFCLASS32 = 1,
    ELFCLASS64 = 2
  };

  enum DataType {
    ELFDATANONE = 0,
    ELFDATA2LSB = 1,
    ELFDATA2MSB = 2
  };

  enum ABIType {
    ELFOSABI_NONE = 0, // No extensions or unspecified
    ELFOSABI_HPUX = 1, // Hewlett-Packard HP-UX
    ELFOSABI_NETBSD = 2, // NetBSD
    ELFOSABI_LINUX = 3, // Linux
    ELFOSABI_SOLARIS = 6, // Sun Solaris
    ELFOSABI_AIX = 7, // AIX
    ELFOSABI_IRIX = 8, // IRIX
    ELFOSABI_FREEBSD = 9, // FreeBSD
    ELFOSABI_TRU64 = 10, // Compaq TRU64 UNIX
    ELFOSABI_MODESTO = 11, // Novell Modesto
    ELFOSABI_OPENBSD = 12, // Open BSD
    ELFOSABI_OPENVMS = 13, // Open VMS
    ELFOSABI_NSK = 14, // Hewlett-Packard Non-Stop Kernel
    ELFOSABI_AROS = 15, // Amiga Research OS
    ELFOSABI_FENIXOS = 16, // The FenixOS highly scalable multi-core OS
  };

  enum Type {
    ET_NONE = 0,
    ET_REL  = 1,
    ET_EXEC = 2,
    ET_DYN = 3,
    ET_CORE = 4
  };

  enum ProgramHeaderType {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC= 2,
    PT_INTERP= 3,
    PT_NOTE= 4,
    PT_SHLIB = 5,
    PT_PHDR= 6,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff
  };

  enum ProgramHeaderFlags {
    PF_R = 0x4,
    PF_W = 0x2,
    PF_X = 0x1
  };

  enum MachineType {
    MT_NONE = 0,
    MT_X86  = 3,
    MT_X86_64 = 62
  };

  /*
   * Program header 
   */
  struct ProgramHeader {
    u32 p_type;   /* Segment type: Loadable segment = 1 */
    u32 p_offset; /* Offset of segment in file */
    u32 p_vaddr;  /* Reqd virtual address of segment  when loading */
    u32 p_paddr;  /* Reqd physical address of segment (ignore) */
    u32 p_filesz; /* How many bytes this segment occupies in file */
    u32 p_memsz;  /* How many bytes this segment should occupy in memory
                     (when loading, expand the segment by
                      concatenating enough zero bytes to it) */
    u32 p_flags;  /* Flags: logical "or" of PF_ constants below */
    u32 p_align;  /* Reqd alignment of segment in memory */

    bool load_p() {
      return p_type == PT_LOAD;
    }

    bool readable_p() {
      return (p_flags & PF_R) == PF_R;
    }

    bool writable_p() {
      return (p_flags & PF_W) == PF_W;
    }

    bool executable_p() {
      return (p_flags & PF_X) == PF_X;
    }

    int mmap_flags() {
      int flags = 0;
      if(readable_p()) flags |= MemoryMapping::eReadable;
      if(writable_p()) flags |= MemoryMapping::eWritable;
      if(executable_p()) flags |= MemoryMapping::eExecutable;

      return flags;
    }

  };


  struct Header {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;
    u32 e_phoff;
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;

    bool validate();

    int ph_size() const {
      return e_phentsize * e_phnum;
    }

    ProgramHeader* load_ph(fs::Node*);
  };

  /*
   * Section header 
   */
  struct Section {
    u32        sh_name;
    u32        sh_type;
    u32        sh_flags;
    u32        sh_addr;
    u32        sh_offset;
    u32        sh_size;
    u32        sh_link;
    u32        sh_info;
    u32        sh_addralign;
    u32        sh_entsize;
  };

  struct Request {
    const char* path;
    const char** argv;
    const char** env;
    fs::Node* node;
    u32 new_esp;
    u32 target_ip;

    Request(const char* p, const char** a, const char** e)
      : path(p)
      , argv(a)
      , env(e)
      , node(0)
      , new_esp(0)
      , target_ip(0)
    {}

    bool load_file();
  };

  bool load_node(Request& req);

}

#endif
