#ifndef INSPECTOR_HPP
#define INSPECTOR_HPP

#include "elf.hpp"
#include "multiboot.hpp"

class Inspector {
  const char* strtab_;
  int strtab_size_;

  elf::Symbol* symtab_;
  int symtab_size_;

public:
  void init(multiboot* mboot);
  void print_backtrace();
  void print_backtrace(u32** ebp);
  const char* resolve_symbol(u32 addr, u32* offset);
};

extern Inspector inspector;

#endif
