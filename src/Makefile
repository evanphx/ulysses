# Makefile for JamesM's kernel tutorials.
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler 
# rule, as we use nasm instead of GNU as.

SOURCES=boot.o main.o monitor.o common.o descriptor_tables.o isr.o interrupt.o gdt.o timer.o \
        kheap.o paging.o ordered_array.o fs.o initrd.o task.o process.o syscall.o \
				keyboard.o pci.o

CFLAGS=-nostdlib -nostdinc -fno-builtin
LDFLAGS=-Tlink.ld
ASFLAGS=-felf

CC=i386-elf-gcc
LD=i386-elf-ld

all: $(SOURCES) link

clean:
	-rm *.o kernel

link:
	$(LD) $(LDFLAGS) -o kernel $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<
