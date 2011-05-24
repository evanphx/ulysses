;
; boot.s -- Kernel start location. Also defines multiboot header.
;           Based on Bran's kernel development tutorial file start.asm
;

MBOOT_PAGE_ALIGN    equ 1<<0    ; Load kernel and modules on a page boundary
MBOOT_MEM_INFO      equ 1<<1    ; Provide your kernel with memory info
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; Multiboot Magic value
; NOTE: We do not use MBOOT_AOUT_KLUDGE. It means that GRUB does not
; pass us a symbol table.
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

KERNEL_VIRTUAL_BASE equ 0xC0000000                  ; 3GB
KERNEL_PAGE_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22)  ; Page directory index of kernel's 4MB PTE.

STACKSIZE equ 0x4000

[BITS 32]                       ; All instructions should be 32-bit.

[GLOBAL mboot]                  ; Make 'mboot' accessible from C.
[EXTERN code]                   ; Start of the '.text' section.
[EXTERN bss]                    ; Start of the .bss section.
[EXTERN end]                    ; End of the last loadable section.

[EXTERN kernel_start]
[EXTERN kernel_end]

mboot:
    dd  MBOOT_HEADER_MAGIC      ; GRUB will search for this value on each
                                ; 4-byte boundary in your kernel file
    dd  MBOOT_HEADER_FLAGS      ; How GRUB should load your file / settings
    dd  MBOOT_CHECKSUM          ; To ensure that the above values are correct
    
[GLOBAL start]                  ; Kernel entry point.
[EXTERN kmain]                   ; This is the entry point of our C code

start:
    ; NOTE: Until paging is set up, the code must be position-independent
    ; and use physical addresses, not virtual ones!
    mov ecx, (BootPageDirectory - KERNEL_VIRTUAL_BASE)
    mov cr3, ecx         ; Load Page Directory Base Register.

    mov ecx, cr4
    or ecx, 0x00000010   ; Set PSE bit in CR4 to enable 4MB pages.
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80000000   ; Set PG bit in CR0 to enable paging.
    mov cr0, ecx

    ; Start fetching instructions in kernel space.
    lea ecx, [StartInHigherHalf]
    jmp ecx              ; NOTE: Must be absolute jump!

StartInHigherHalf:
    ; Unmap the identity-mapped first 4MB of physical address space.
    ; It should not be needed anymore.
    ; mov dword [BootPageDirectory], 0
    ; invlpg [0]

    ; NOTE: From now on, paging should be enabled. The first 4MB of
    ; physical address space is mapped starting at KERNEL_VIRTUAL_BASE.
    ; Everything is linked to this address, so no more
    ; position-independent code or funny business with virtual-to-physical
    ; address translation should be necessary. We now have a higher-half kernel.
    mov esp, stack+STACKSIZE    ; set up the stack

    push kernel_end
    push kernel_start

    push eax                    ; pass Multiboot magic number

    ; Load multiboot information:
    push ebx

    ; Execute the kernel:
    cli                         ; Disable interrupts.
    call kmain                  ; call our main() function.

crazy:
    pause
    pause
    pause
    pause
    jmp crazy                   ; Enter an infinite loop, to stop the processor
                                ; executing whatever rubbish is in the memory
                                ; after our kernel!

align 0x1000
BootPageDirectory:
    ; This page directory entry identity-maps the first 4MB of the
    ; 32-bit physical address space.
    ; All bits are clear except the following:
    ; bit 7: PS The kernel page is 4MB.
    ; bit 1: RW The kernel page is read/write.
    ; bit 0: P  The kernel page is present.
    ; This entry must be here -- otherwise the kernel will crash
    ; immediately after paging is enabled because it can't fetch
    ; the next instruction! It's ok to unmap this page later.

    dd 0x00000083
    times (KERNEL_PAGE_NUMBER - 1) dd 0         ; Pages before kernel space.
    ; This page directory entry defines a 4MB page containing the kernel.
    dd 0x00000083
    times (1024 - KERNEL_PAGE_NUMBER - 1) dd 0  ; Pages after the kernel image.

section .bss
align 32
stack:
    resb STACKSIZE      ; reserve 16k stack on a quadword boundary
