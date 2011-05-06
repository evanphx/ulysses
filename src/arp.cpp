#include "monitor.hpp"
#include "arp.hpp"
#include "eth.hpp"
#include "rtl8139.hpp"
#include "kheap.hpp"

struct arp_header {
  u16int hardware;
  u16int proto;
  u8int hardware_len;
  u8int proto_len;
  u16int op;
} __attribute__((packed));

#define ARP_ETHER 1

#define ARP_REQUEST 1
#define ARP_REPLY 2
#define RARP_REQUEST 3
#define RARP_REPLY 4

void handle_arp(u8int* buf, int size) {
  struct arp_header* hdr = (struct arp_header*)buf;

  u16int hlen = hdr->hardware_len;
  u16int plen = hdr->proto_len;

  console.write("ARP packet:  htype=");
  console.write_hex(htons(hdr->hardware));
  console.write(" proto=");
  console.write_hex(htons(hdr->proto));
  console.write(" hlen=");
  console.write_hex(hdr->hardware_len);
  console.write(" plen=");
  console.write_hex(hdr->proto_len);
  console.write(" op=");
  console.write(htons(hdr->op) == ARP_REQUEST ? "request" : "reply");
  console.write("\n");

  if(htons(hdr->hardware) == ARP_ETHER &&
      htons(hdr->proto) == PROTO_IP) {
    u8int* var = buf + sizeof(struct arp_header);
    u8int* sha = var;
    u8int* spa = sha + hlen;
    u8int* tha = spa + plen;
    u8int* tpa = tha + hlen;

    console.write("sender: ");
    print_mac(sha);
    console.write(" - ");
    print_ipv4(spa);

    console.write("\n");

    console.write("target: ");
    print_mac(tha);
    console.write(" - ");
    print_ipv4(tpa);

    console.write("\n");

    int rep_size = sizeof(struct eth_header) + sizeof(struct arp_header) + 6+4+6+4;
    u8int* reply = (u8int*)kmalloc_a(rep_size);

    struct eth_header* rep_hdr = (struct eth_header*)reply;
    memcpy(rep_hdr->dest, sha, 6);
    memcpy(rep_hdr->src, (u8int*)eth_mac(), 6);
    rep_hdr->proto = htons(PROTO_ARP);

    struct arp_header* rep_arp_hdr = (struct arp_header*)(rep_hdr + 1);
    memcpy((u8int*)rep_arp_hdr, (u8int*)hdr, sizeof(struct arp_header));
    rep_arp_hdr->op = htons(ARP_REPLY);

    u8int* body = (u8int*)(rep_arp_hdr + 1);
    memcpy((u8int*)body + 0,  (u8int*)eth_mac(), 6);
    memcpy(body + 6,  tpa, 4);
    memcpy(body + 10, sha, 6);
    memcpy(body + 16, spa, 4);

    xmit_packet(reply, rep_size);
    kputs("Sent ARP reply\n");
  }
}
