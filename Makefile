all:
	nasm -f elf32 boot/boot.asm -o boot/boot.o
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
	ld -m elf_i386 -T linker.ld -o iso/boot/myos.bin \
		boot/boot.o kernel/vga.o kernel/fs.o kernel/auth.o \
		kernel/shell.o kernel/kernel.o kernel/wm.o kernel/rtc.o kernel/net.o kernel/pci.o kernel/rtl8139.o kernel/process.o kernel/security.o kernel/syscall.o kernel/elf.o kernel/win32.o kernel/pe.o
	grub-mkrescue -o myos.iso iso

clean:
	rm -f boot/*.o kernel/*.o iso/boot/myos.bin myos.iso
