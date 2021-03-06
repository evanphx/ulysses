// common.c -- Defines some global functions.
//             From JamesM's kernel development tutorials.

#include "common.hpp"
#include "monitor.hpp"
#include "cpu.hpp"
#include "inspector.hpp"

extern "C" {

// Write a byte out to the specified port.
void outb(u16int port, u8 value) {
  asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

// Write a byte out to the specified port.
void outw(u16int port, u16int value) {
  asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

// Write a byte out to the specified port.
void outl(u16int port, u32 value) {
  asm volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

u8 inb(u16int port) {
  u8 ret;
  asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

u16int inw(u16int port) {
  u16int ret;
  asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

u32 inl(u16int port) {
  u32 ret;
  asm volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

void insl(u16 port, u32 buffer, u32 count) {
  while(count--) {
    *(u32*)buffer = inl(port);
    buffer += 4;
  }
}

// Copy len bytes from src to dest.
void memcpy(u8 *dest, const u8 *src, u32 len) {
  for(; len != 0; len--) {
    *dest++ = *src++;
  }
}

// Write len copies of val into dest.
void memset(u8 *dest, u8 val, u32 len) {
  for(u8* tmp = dest; len != 0; len--) {
    *tmp++ = val;
  }
}

// Compare two strings. Should return -1 if 
// str1 < str2, 0 if they are equal or 1 otherwise.
int strcmp(const char *str1, const char *str2) {
  int i = 0;
  int failed = 0;
  while(str1[i] != '\0' && str2[i] != '\0') {
    if(str1[i] != str2[i]) {
      failed = 1;
      break;
    }
    i++;
  }

  // why did the loop exit?
  if( (str1[i] == '\0' && str2[i] != '\0') ||
      (str1[i] != '\0' && str2[i] == '\0') ) {
    failed = 1;
  }

  return failed;
}

int strncmp(const char* str1, const char* str2, int len) {
  int i = 0;

  while(i < len) {
    if(str1[i] == 0) {
      if(str2[i] == 0) return 0;
      return -1;
    }

    if(str1[i] != str2[i]) return 1;
    i++;
  }

  return 0;
}

// Copy the NULL-terminated string src into dest, and
// return dest.
char *strcpy(char *dest, const char *src) {
  do {
    *dest++ = *src++;
  } while(*src != 0);

  return dest;
}

// Concatenate the NULL-terminated string src onto
// the end of dest, and return dest.
char *strcat(char *dest, const char *src) {
  while(*dest != 0) {
    char c = *dest++;
    *dest = c;
  }

  do {
    *dest++ = *src++;
  } while(*src != 0);
  return dest;
}

int strlen(const char* str) {
  int i = 0;
  while(str[i]) i++;
  return i;
}

extern void panic(const char *message, const char *file, u32 line)
{
    // We encountered a massive problem and have to stop.
    cpu::disable_interrupts();

    console.printf("PANIC(%s) at %s:%d\n", message, file, line);

    inspector.print_backtrace();

    // Halt by going into an infinite loop.
    cpu::halt_loop();
}

extern void panic_assert(const char *file, u32 line, const char *desc)
{
    // An assertion failed, and we have to panic.
    cpu::disable_interrupts();

    console.printf("ASSERTION-FAILED(%s) at %s:%d\n", desc, file, line);

    inspector.print_backtrace();

    // Halt by going into an infinite loop.
    cpu::halt_loop();
}

void kabort() {
  cpu::disable_interrupts();
  kputs("Your kernel has aborted(). Get some cofffe.\n");
  inspector.print_backtrace();
  cpu::halt_loop();
}

void __cxa_pure_virtual() {
  cpu::disable_interrupts();
  console.printf("Pure virtual method called. Full stop.\n");
  inspector.print_backtrace();
  cpu::halt_loop();
}

void __cxa_guard_acquire() {}
void __cxa_guard_release() {}

}
