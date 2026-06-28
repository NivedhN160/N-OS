#ifndef RTL8139_H
#define RTL8139_H

void rtl8139_init(unsigned char bus, unsigned char slot, unsigned char func);
void rtl8139_send_packet(void *data, int len);

#endif
