all:
	cd src; make

libc: sys/.libc-configured
	cd sys/libc; make

sys/.libc-configured: sys/libc
	cd sys/libc; ./configure
	touch sys/.libc-configured

sys/libc:
	cd sys; git clone git@github.com:evanphx/ulysses-libc.git libc

test:
	cd sys; gcc -nostdlib -nostdinc -ggdb -O0 -I libc/include/ -o test test.c libc/lib/libc.a /usr/lib/gcc/i686-linux-gnu/4.6/libgcc.a; tar czvf tar_disk test

full: libc test all
