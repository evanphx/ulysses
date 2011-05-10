# Makefile for JamesM's kernel tutorials.
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler 
# rule, as we use nasm instead of GNU as.

COBJECTS=main.o console.o common.o descriptor_tables.o isr.o timer.o \
        kheap.o paging.o ordered_array.o fs.o initrd.o task.o syscall.o \
				keyboard.o pci.o rtl8139.o eth.o arp.o rtc.o pit.o elf.o

SOBJECTS=boot.o interrupt.o gdt.o process.o

SOURCES=$(COBJECTS:.o=.cpp) $(SOBJECTS:.o=.s)
OBJECTS=$(COBJECTS) $(SOBJECTS)

OPT=-O0 -fno-inline
CXXFLAGS=$(OPT) -g -ggdb3 -Wall -Werror -fno-rtti -fno-exceptions -nostdlib -fno-builtin -Ilwip/src/include -Ilwip/src/include/ipv4 -Ilwip/arch -Ilwip
LIBGCC=`i386-elf-gcc -print-libgcc-file-name`
LDFLAGS=--oformat=elf32-i386 -melf_i386 -Tlink.ld
ASFLAGS=-felf

CC=../buildtools/local/bin/i386-pc-ulysses-g++
CXX=../buildtools/local/bin/i386-pc-ulysses-g++

LD=../buildtools/local/bin/i386-pc-ulysses-ld

all: link

dep:
	$(CC) $(CXXFLAGS) -MM $(SOURCES) > depend

clean:
	-rm *.o kernel

link: $(OBJECTS)
	cd lwip; make
	$(LD) $(LDFLAGS) -o kernel $(OBJECTS) liblwip4.a $(LIBGCC)

.s.o:
	nasm $(ASFLAGS) $<
# DO NOT DELETE

-include depend
