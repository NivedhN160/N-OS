CC = gcc
CFLAGS = -m32 -ffreestanding -Wall -Wextra -Ikernel/arch -Ikernel/drivers -Ikernel/mm -Ikernel/fs -Ikernel/net -Ikernel/sched -Ikernel/gui -Ikernel/loader -Ikernel/syscall -Ikernel/security -Ikernel/gui/apps -Ikernel
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL_DIR = kernel
BOOT_DIR = boot

C_SOURCES = $(shell find $(KERNEL_DIR) -name '*.c')
C_OBJS = $(C_SOURCES:.c=.o)

ASM_SOURCES = $(wildcard $(BOOT_DIR)/*.asm)
ASM_OBJS = $(ASM_SOURCES:.asm=.o)

ALL_OBJS = $(ASM_OBJS) $(C_OBJS)

all: myos.iso

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm -f elf32 $< -o $@

iso/boot/myos.bin: $(ALL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)

myos.iso: iso/boot/myos.bin
	grub-mkrescue -o $@ iso

clean:
	rm -f $(ALL_OBJS) iso/boot/myos.bin myos.iso
