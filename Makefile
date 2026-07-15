all:
	nasm -f elf32 boot/boot.asm -o boot/boot.o
	nasm -f elf32 boot/interrupt.asm -o boot/interrupt.o
	gcc -m32 -ffreestanding -c kernel/vga.c -o kernel/vga.o
	gcc -m32 -ffreestanding -c kernel/fs.c -o kernel/fs.o
	gcc -m32 -ffreestanding -c kernel/auth.c -o kernel/auth.o
	gcc -m32 -ffreestanding -c kernel/shell.c -o kernel/shell.o
	gcc -m32 -ffreestanding -c kernel/kernel.c -o kernel/kernel.o
	gcc -m32 -ffreestanding -c kernel/wm.c -o kernel/wm.o
	gcc -m32 -ffreestanding -c kernel/rtc.c -o kernel/rtc.o
	gcc -m32 -ffreestanding -c kernel/net.c -o kernel/net.o
	gcc -m32 -ffreestanding -c kernel/pci.c -o kernel/pci.o
	gcc -m32 -ffreestanding -c kernel/rtl8139.c -o kernel/rtl8139.o
	gcc -m32 -ffreestanding -c kernel/process.c -o kernel/process.o
	gcc -m32 -ffreestanding -c kernel/security.c -o kernel/security.o
	gcc -m32 -ffreestanding -c kernel/syscall.c -o kernel/syscall.o
	gcc -m32 -ffreestanding -c kernel/elf.c -o kernel/elf.o
	gcc -m32 -ffreestanding -c kernel/win32.c -o kernel/win32.o
	gcc -m32 -ffreestanding -c kernel/pe.c -o kernel/pe.o
	gcc -m32 -ffreestanding -c kernel/kmalloc.c -o kernel/kmalloc.o
	gcc -m32 -ffreestanding -c kernel/rng.c -o kernel/rng.o
	gcc -m32 -ffreestanding -c kernel/gdt.c -o kernel/gdt.o
	gcc -m32 -ffreestanding -c kernel/idt.c -o kernel/idt.o
	gcc -m32 -ffreestanding -c kernel/isr.c -o kernel/isr.o
	gcc -m32 -ffreestanding -c kernel/paging.c -o kernel/paging.o
	gcc -m32 -ffreestanding -c kernel/pit.c -o kernel/pit.o
	gcc -m32 -ffreestanding -c kernel/panic.c -o kernel/panic.o
	gcc -m32 -ffreestanding -c kernel/block.c -o kernel/block.o
	gcc -m32 -ffreestanding -c kernel/ata.c -o kernel/ata.o
	gcc -m32 -ffreestanding -c kernel/vfs.c -o kernel/vfs.o
	gcc -m32 -ffreestanding -c kernel/fat32.c -o kernel/fat32.o
	gcc -m32 -ffreestanding -c kernel/compositor.c -o kernel/compositor.o
	ld -m elf_i386 -T linker.ld -o iso/boot/myos.bin \
		boot/boot.o boot/interrupt.o kernel/vga.o kernel/fs.o kernel/auth.o \
		kernel/shell.o kernel/kernel.o kernel/wm.o kernel/rtc.o kernel/net.o kernel/pci.o kernel/rtl8139.o kernel/process.o kernel/security.o kernel/syscall.o kernel/elf.o kernel/win32.o kernel/pe.o kernel/kmalloc.o kernel/rng.o kernel/gdt.o kernel/idt.o kernel/isr.o kernel/paging.o kernel/pit.o kernel/panic.o kernel/block.o kernel/ata.o kernel/vfs.o kernel/fat32.o kernel/compositor.o
	grub-mkrescue -o myos.iso iso

clean:
	rm -f boot/*.o kernel/*.o iso/boot/myos.bin myos.iso
