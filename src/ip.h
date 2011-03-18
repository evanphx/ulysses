struct ip_header {
  u8int ihl:4;
  u8int version:4;
	u8int	tos;
	u16int tot_len;
	u16int id;
	u16int frag_off;
	u8int	ttl;
	u8int protocol;
	u16int check;
	u32int saddr;
	u32int daddr;
} __attribute__((packed));

struct tcp_header {
	u16int source;
	u16int dest;
	u32int seq;
	u32int ack_seq;
  u16int flags;
	u16int window;
	u16int check;
  u16int urgent;
} __attribute__((packed));

void handle_ip(u8int* buf, int size, char* src_mac);
