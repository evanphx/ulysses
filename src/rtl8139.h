void init_rtl8139(u32int io_port, u8int irq);
void xmit_packet(u8int* buf, int size);
char* eth_mac();
