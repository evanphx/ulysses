
struct eth_header {
  u8int dest[6];
  u8int src[6];
  u16int proto;
} __attribute__((packed));

#define PROTO_ARP 0x806
#define PROTO_IP 0x800

void eth_handoff(u8int* buf, int size);

void print_mac(u8int* buf);

void print_ipv4(u8int* buf);
