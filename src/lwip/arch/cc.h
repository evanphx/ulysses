#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#define BYTE_ORDER  LITTLE_ENDIAN

typedef unsigned int   u32_t;
typedef          int   s32_t;
typedef unsigned short u16_t;
typedef          short s16_t;
typedef unsigned char  u8_t;
typedef          char  s8_t;


typedef unsigned long mem_ptr_t;

extern void kprintf(char* fmt, ...);
extern void kabort();

extern void* memcpy(void* s, const void* d, int size);
extern void* memset(void* b, int c, int len);

#define LWIP_ERR_T  int

/* Define (sn)printf formatters for these lwIP types */
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

/* Compiler hints for packing structures */
#define PACK_STRUCT_FIELD(x)    x
#define PACK_STRUCT_STRUCT  __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)   do {                \
        kprintf x;                   \
    } while (0)

#define LWIP_PLATFORM_ASSERT(x) do {                \
        kprintf("Assert \"%s\" failed at line %d in %s\n",   \
                x, __LINE__, __FILE__);             \
        kabort();                        \
    } while (0)

#endif /* __ARCH_CC_H__ */

