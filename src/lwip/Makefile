CC=../../buildtools/local/bin/i386-pc-ulysses-gcc
LD=../../buildtools/local/bin/i386-pc-ulysses-ld
AR=../../buildtools/local/bin/i386-pc-ulysses-ar

CCDEP=$(CC)

#CFLAGS += -DLWIP_DEBUG
ARFLAGS=rs

TOPDIR=.
LWIPARCH=$(TOPDIR)/arch

#Set this to where you have the lwip core module checked out from CVS
#default assumes it's a dir named lwip at the same level as the contrib module
LWIPDIR=$(TOPDIR)/src

CFLAGS += -m32 -Wall -Werror $(CPPFLAGS) -I$(LWIPDIR)/include -I.              \
    -I$(LWIPARCH) \
    -I$(LWIPDIR)/include/ipv4

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES=$(LWIPDIR)/core/mem.c             \
    $(LWIPDIR)/core/memp.c              \
    $(LWIPDIR)/core/netif.c             \
    $(LWIPDIR)/core/pbuf.c              \
    $(LWIPDIR)/core/raw.c               \
    $(LWIPDIR)/core/stats.c             \
    $(LWIPDIR)/core/sys.c               \
    $(LWIPDIR)/core/tcp.c               \
    $(LWIPDIR)/core/tcp_in.c            \
    $(LWIPDIR)/core/tcp_out.c           \
    $(LWIPDIR)/core/udp.c               \
    $(LWIPDIR)/core/dhcp.c              \
    $(LWIPDIR)/core/init.c

CORE4FILES=$(LWIPDIR)/core/ipv4/icmp.c          \
    $(LWIPDIR)/core/ipv4/ip.c           \
    $(LWIPDIR)/core/ipv4/inet.c         \
    $(LWIPDIR)/core/ipv4/ip_addr.c          \
    $(LWIPDIR)/core/ipv4/ip_frag.c          \
    $(LWIPDIR)/core/ipv4/inet_chksum.c

# NETIFFILES: Files implementing various generic network interface functions.'
NETIFFILES=$(LWIPDIR)/netif/etharp.c            

# LWIPFILES: All the above.
LWIPFILES=$(COREFILES) $(CORE4FILES) $(NETIFFILES)
OBJS=$(notdir $(LWIPFILES:.c=.o))

LWIPLIB=liblwip4.a

all compile: $(LWIPLIB)
	cp $(LWIPLIB) ../

.PHONY: all depend compile clean

%.o:
	$(CC) $(CFLAGS) -c $(<:.o=.c)

clean:
	rm -f *.o $(LWIPLIB) .depend*

$(LWIPLIB): $(OBJS)
	$(AR) $(ARFLAGS) $(LWIPLIB) $?

depend dep: .depend

include .depend

.depend: $(LWIPFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend || rm -f .depend

