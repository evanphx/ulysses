#include "monitor.h"
#include "eth.h"
#include "arp.h"

void print_mac(u8int* buf) {
  int i;
  for(i = 0; i < 5; i++) {
    monitor_write_hex_byte(buf[i]);
    monitor_write(":");
  }
  monitor_write_hex_byte(buf[5]);
}

void print_ipv4(u8int* buf) {
  int i;
  for(i = 0; i < 3; i++) {
    monitor_write_dec(buf[i]);
    monitor_write(".");
  }
  monitor_write_dec(buf[3]);
}

void eth_handoff(u8int* buf, int size) {
  int i;
  struct eth_header* hdr = (struct eth_header*)buf;

  monitor_write("ethernet frame:  dest=");
  for(i = 0; i < 5; i++) {
    monitor_write_hex_byte(hdr->dest[i]);
    monitor_write(":");
  }
  monitor_write_hex_byte(hdr->dest[5]);

  monitor_write(" src=");
  for(i = 0; i < 5; i++) {
    monitor_write_hex_byte(hdr->src[i]);
    monitor_write(":");
  }
  monitor_write_hex_byte(hdr->src[5]);

  u16int proto = htons(hdr->proto);
  monitor_write(" proto=");
  monitor_write_hex(proto);
  monitor_write("\n");

  /*
  if(proto == PROTO_ARP) {
    handle_arp(buf + sizeof(struct eth_header),
               size - sizeof(struct eth_header));
  } else if(proto == PROTO_IP) {
    handle_ip(buf + sizeof(struct eth_header),
               size - sizeof(struct eth_header),
               hdr->src);
  }
  */
}
