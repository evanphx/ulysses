all: kernel

kernel:
	cd src; make

libc: sys/.libc-configured
	cd sys/libc; make install

sys/.libc-configured: sys/libc
	cd sys/libc; ./configure --prefix=`pwd`/../local
	touch sys/.libc-configured

sys/libc:
	cd sys; git clone git@github.com:evanphx/ulysses-libc.git libc

test: sys/tar_disk


LCC=./local/bin/musl-gcc

sys/tar_disk: sys/test
	cd sys; tar czvf tar_disk test

sys/test: sys/test.c
	cd sys; ${LCC} -static -o test test.c

sys/test-dyn:
	cd sys; ${LCC} -o test-dyn test.c

run:
	qemu-system-x86_64 -kernel src/kernel -initrd sys/tar_disk -hda scratch/words_disk

full: libc test all
