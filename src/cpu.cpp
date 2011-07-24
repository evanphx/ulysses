#include "common.hpp"
#include "cpu.hpp"
#include "console.hpp"

namespace cpu {
  void print_cpuid() {
    u32 ebx, edx, ecx;
    u32 code = 0;

    asm volatile("cpuid" : "=b"(ebx),"=d"(edx),"=c"(ecx) : "a"(code));

    console.write("CPUID: ");
    console.put((ebx & 0x000000ff) >> 0);
    console.put((ebx & 0x0000ff00) >> 8);
    console.put((ebx & 0x00ff0000) >> 16);
    console.put((ebx & 0xff000000) >> 24);
    console.put((edx & 0x000000ff) >> 0);
    console.put((edx & 0x0000ff00) >> 8);
    console.put((edx & 0x00ff0000) >> 16);
    console.put((edx & 0xff000000) >> 24);
    console.put((ecx & 0x000000ff) >> 0);
    console.put((ecx & 0x0000ff00) >> 8);
    console.put((ecx & 0x00ff0000) >> 16);
    console.put((ecx & 0xff000000) >> 24);
    console.write("\n");
  }
}
