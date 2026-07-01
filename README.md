# N-OS v1.1

N-OS is a lightweight, custom 32-bit bare-metal Operating System built entirely from scratch in C. It features a custom graphical user interface, a virtual file system, a TCP networking stack, and a universal binary compatibility layer capable of natively executing Windows Portable Executable (PE) and Linux ELF binaries!

## Features

### 🖥️ Hardware GPU Detection & Peach Graphics
During the boot sequence, N-OS scans the PCI bus. If a Class `0x03` Display Controller (GPU) is detected, hardware-acceleration mode is enabled, transforming the standard blue desktop into a high-fidelity Peach Graphic gradient!

### ⚙️ Win32 & POSIX Compatibility Layer
N-OS features a custom native implementation of core OS APIs:
- **Win32 Layer**: Implements `CreateWindowExA` inside N-OS. When a loaded Windows `.exe` calls this function, it is intercepted and bridged seamlessly to the native N-OS window manager!
- **POSIX Syscalls**: Mocked `int 0x80` handlers for `sys_write` and `sys_exit` to support basic Linux `.elf` execution.

### 📦 Universal Binary Execution (PE & ELF)
N-OS does not rely on simple shell mocks. It features real binary loaders:
- **PE Loader (`pe.c`)**: Parses DOS `MZ` signatures and Windows `PE` headers, extracting sections and executing Windows `.exe` files directly in memory.
- **ELF Loader (`elf.c`)**: Parses `\x7fELF` magic headers and `PT_LOAD` segments for Linux binaries.

### 🌐 Custom TCP/IP Stack
A from-scratch networking stack (`net.c`) built on top of the RTL8139 ethernet driver. It handles Ethernet framing, IPv4 packet construction, and TCP 3-way handshakes (SYN, SYN-ACK, ACK) to successfully fetch HTTP payloads!

### 🛒 App Store & Package Management
A fully functional GUI App Store and terminal Package Manager (`pkg`). Applications like Firefox and Calculator are not installed by default. You can install them dynamically via `pkg install firefox`, which updates the Virtual File System and dynamically populates the Start Menu!

## Installation & Download

You can download the pre-compiled, bootable `.iso` image directly from this repository and run it in any virtual machine!

**Download ISO**: [myos.iso](myos.iso?raw=true)

### Running in VirtualBox
1. Open VirtualBox and create a new Virtual Machine.
2. Set the Type to **Other** and Version to **Other/Unknown (32-bit)**.
3. Allocate at least 64MB of RAM.
4. Go to Settings > Storage > Add Optical Drive, and select the downloaded `myos.iso`.
5. Under Settings > Network, set the adapter type to **PCnet-FAST III (Am79C973)** or **Intel PRO/1000 MT Desktop** (RTL8139 emulation).
6. Start the VM and enjoy N-OS!

## Compiling from Source

If you want to modify N-OS and compile it yourself, you will need a standard GCC cross-compiler toolchain and `xorriso`.

```bash
# Compile the OS and generate the ISO
make
```

## Available Terminal Commands
- `help` - Show all commands
- `pkg install <app>` - Install applications
- `run <app.exe>` - Execute a Windows PE binary via the compatibility layer
- `clear` - Clear the terminal
- `bg` / `fg` / `kill` - Process management
