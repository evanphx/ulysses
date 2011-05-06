#include "monitor.hpp"
#include "eth.hpp"
#include "arp.hpp"

void print_mac(u8int* buf) {
  int i;
  for(i = 0; i < 5; i++) {
    console.write_hex_byte(buf[i]);
    console.write(":");
  }
  console.write_hex_byte(buf[5]);
}

void print_ipv4(u8int* buf) {
  int i;
  for(i = 0; i < 3; i++) {
    console.write_dec(buf[i]);
    console.write(".");
  }
  console.write_dec(buf[3]);
}

void eth_handoff(u8int* buf, int size) {
  int i;
  struct eth_header* hdr = (struct eth_header*)buf;

  console.write("ethernet frame:  dest=");
  for(i = 0; i < 5; i++) {
    console.write_hex_byte(hdr->dest[i]);
    console.write(":");
  }
  console.write_hex_byte(hdr->dest[5]);

  console.write(" src=");
  for(i = 0; i < 5; i++) {
    console.write_hex_byte(hdr->src[i]);
    console.write(":");
  }
  console.write_hex_byte(hdr->src[5]);

  u16int proto = htons(hdr->proto);
  console.write(" proto=");
  console.write_hex(proto);
  console.write("\n");

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
