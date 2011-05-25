// common.h -- Defines typedefs and some global functions.
//             From JamesM's kernel development tutorials.

#ifndef COMMON_H
#define COMMON_H

extern "C" {

// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86.
typedef unsigned long long u64;
typedef          long long s64;
typedef unsigned int   u32int;
typedef          int   s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

typedef unsigned int u32;
typedef          int s32;

typedef unsigned short u16;
typedef          short s16;

typedef unsigned char u8;
typedef          char s8;

void outb(u16int port, u8int value);
void outw(u16int port, u16int value);
void outl(u16int port, u32int value);

u8int inb(u16int port);
u16int inw(u16int port);
u32int inl(u16int port);

void insl(u16 port, u32 buffer, u32 size);

#define PANIC(msg) panic(msg, __FILE__, __LINE__);
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))

extern void panic(const char *message, const char *file, u32int line);
extern void panic_assert(const char *file, u32int line, const char *desc);

void memset(u8int *dest, u8int val, u32int len);
void memcpy(u8int *dest, const u8int *src, u32int len);

char *strcpy(char *dest, const char *src);
int strlen(const char *src);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, int len);

void kabort();

int disable_interrupts();
void restore_interrupts(int status);

#define htons(A) ((((u16int)(A) & 0xff00) >> 8) | \
                  (((u16int)(A) & 0x00ff) << 8))
#define htonl(A) ((((u32int)(A) & 0xff000000) >> 24) | \
                  (((u32int)(A) & 0x00ff0000) >> 8)  | \
                  (((u32int)(A) & 0x0000ff00) << 8)  | \
                  (((u32int)(A) & 0x000000ff) << 24))


}

template <typename T>
  static inline T align(T val, int count) {
    return (val + count - 1) & ~(count - 1);
  }

#endif // COMMON_H
