
#include "common.hpp"
#include "inspector.hpp"
#include "elf.hpp"
#include "console.hpp"

void Inspector::init(multiboot* mboot) {
  return;
  elf::Section* sh = (elf::Section*)mboot->addr;

  console.printf("section: %p, strstrtab: %x\n", sh, mboot->shndx);
  u32 shstrtab = sh[mboot->shndx].addr();

  for(u32 i = 0; i < mboot->num; i++) {
    const char* name = (const char*)(shstrtab + sh[i].name());
    if(strcmp(name, ".strtab") == 0) {
      strtab_ = (const char*)sh[i].addr();
      strtab_size_ = sh[i].size();
    }

    if(strcmp(name, ".symtab") == 0) {
      symtab_ = (elf::Symbol*)sh[i].addr();
      symtab_size_ = sh[i].size() / sizeof(elf::Symbol);
    }
  }
}

const char* Inspector::resolve_symbol(u32 addr, u32* offset) {
  for(int i = 0; i < symtab_size_; i++) {
    elf::Symbol* sym = symtab_ + i;

    if(!sym->func_p()) continue;

    if(sym->contains_p(addr, offset)) {
      return (const char*)((u32)strtab_ + sym->name);
    }
  }
  return 0;
}

void Inspector::print_backtrace() {
  print_backtrace(read_ebp());
}

void Inspector::print_backtrace(u32** ebp) {
  u32** eip;

  while(ebp) {
    eip = ebp+1;
    console.printf("  [%p] unknown\n", *eip);
    ebp = (u32**)*ebp;
  }
}

Inspector inspector;

