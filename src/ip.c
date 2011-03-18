#include "monitor.h"
#include "ip.h"
#include "eth.h"

static void handle_tcp(u8int* buf, int size, char* src_mac, u32int src_ip, u32int dest_ip);

void handle_ip(u8int* buf, int size, char* src_mac) {
  struct ip_header* hdr = (struct ip_header*)buf;

  kputs("IP Pkt: ihl=");
  monitor_write_dec(hdr->ihl);
  kputs(" ver=");
  monitor_write_dec(hdr->version);
  kputs(" tos=");
  monitor_write_dec(hdr->tos);
  kputs(" tot_len=");
  monitor_write_dec(htons(hdr->tot_len));
  kputs(" id=");
  monitor_write_dec(htons(hdr->id));
  kputs(" frag=");
  monitor_write_dec(htons(hdr->frag_off));
  kputs(" ttl=");
  monitor_write_dec(hdr->ttl);
  kputs(" proto=");
  monitor_write_dec(hdr->protocol);
  kputs(" check=");
  monitor_write_dec(htons(hdr->check));
  kputs("\n src=");
  print_ipv4((u8int*)&hdr->saddr);
  kputs(" dest=");
  print_ipv4((u8int*)&hdr->daddr);
  kputs("\n");

  if(hdr->protocol == 6) {
    handle_tcp(buf + sizeof(struct ip_header),
               size - sizeof(struct ip_header),
               src_mac, hdr->saddr, hdr->daddr);
  }
}

static void handle_tcp(u8int* buf, int size, char* src_mac, u32int src_ip, u32int dest_ip) {
  struct tcp_header* hdr = (struct tcp_header*)buf;
  kputs("TCP Pkt: sport=");
  monitor_write_dec(htons(hdr->source));
  kputs(" dport=");
  monitor_write_dec(htons(hdr->dest));
  kputs(" seq=");
  monitor_write_dec(htonl(hdr->seq));
  kputs(" ack_seq=");
  monitor_write_dec(htonl(hdr->ack_seq));

  kputs(" flags=");
  monitor_write_dec(htons(hdr->flags));

  kputs(" window=");
  monitor_write_dec(htons(hdr->window));
  kputs(" check=");
  monitor_write_dec(htons(hdr->check));

  int flags = htons(hdr->flags);
  int data_offset = (flags >> 12);

  kputs(" offset=");
  monitor_write_dec(data_offset);

  if(data_offset > 5) {
    u8int* opts = buf + 20;
    if(opts[0] == 2 && opts[1] == 4) { // MSS
      u16int mss = *(u16int*)(opts+2);
      mss = htons(mss);
      kputs(" mss=");
      monitor_write_dec(mss);
    }
  }

  kputs("\n");

  // Send a reset!
  int rep_size = sizeof(struct eth_header) + sizeof(struct ip_header) +
                  sizeof(struct tcp_header);
  u8int* reply = (u8int*)kmalloc_a(rep_size);

  struct eth_header* rep_hdr = (struct eth_header*)reply;
  memcpy(rep_hdr->dest, src_mac, 6);
  memcpy(rep_hdr->src, eth_mac(), 6);
  rep_hdr->proto = htons(PROTO_IP);

  struct ip_header* ip_hdr = (struct ip_header*)(rep_hdr + 1);
  ip_hdr->version = 4;
  ip_hdr->ihl = 5;
  ip_hdr->tos = 0;
  ip_hdr->tot_len = htons(rep_size - sizeof(struct eth_header));
  ip_hdr->id = 0;
  ip_hdr->check = 0;
  ip_hdr->ttl = 64;
  ip_hdr->protocol = 6;
  ip_hdr->saddr = dest_ip;
  ip_hdr->daddr = src_ip;

  struct tcp_header* tcp_hdr = (struct tcp_header*)(ip_hdr + 1);
  tcp_hdr->source = hdr->dest;
  tcp_hdr->dest = hdr->source;
  tcp_hdr->seq = hdr->seq;
  tcp_hdr->ack_seq = 0;
  tcp_hdr->flags = (1 << 13) | 5;
  tcp_hdr->window = htons(42);
  tcp_hdr->check = 0;

  xmit_packet(reply, rep_size);
  kputs("Sent TCP Rst\n");
}
