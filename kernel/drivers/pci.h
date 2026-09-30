#ifndef PCI_H
#define PCI_H

unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset);
void pci_init();

#endif
