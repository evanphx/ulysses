/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */
/*            Original file taken from Bran's Kernel Development */
/*            tutorials: http://www.osdever.net/bkerndev/index.php. */

phys_start = start - 0xC0000000;

ENTRY(phys_start)
SECTIONS
{

   /* The kernel will live at 3GB + 1MB in the virtual
      address space, which will be mapped to 1MB in the
      physical address space. */
   . = 0xC0100000;

   kernel_start = .;

    .text : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
        . = ALIGN(4096);
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
    }

    .bss : AT(ADDR(.bss) - 0xC0000000)
    {
        *(.bss)
        . = ALIGN(4096);
    }

    kernel_end = .;
}
