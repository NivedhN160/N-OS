// Removed missing include

// ── POSIX System Call Interface (Mock)
// This implements a lightweight compatibility layer for basic POSIX
// syscalls. This makes it possible (in theory) to run standard
// C programs if compiled against a small C library (like newlib)
// without rewriting them for N-OS specific APIs.

extern void term_print(const char *s, unsigned int col);
extern int slen(const char* s);

// Standard File Descriptors
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// Basic POSIX Syscalls (Interrupt 0x80)
#define SYS_EXIT  1
#define SYS_READ  3
#define SYS_WRITE 4
#define SYS_OPEN  5
#define SYS_CLOSE 6

int sys_write(int fd, const char *buf, int count) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        // N-OS Terminal uses 0xFFFFFF for standard text
        char temp[256];
        int i=0;
        for(; i<count && i<255; i++) temp[i] = buf[i];
        temp[i] = 0;
        term_print(temp, 0xFFFFFF);
        return i;
    }
    return -1; // Bad FD
}

int sys_exit(int status) {
    term_print("Process exited with status ", 0xAAAAAA);
    // Print status logic
    return 0;
}

// POSIX System Call Handler (Mock for int 0x80)
int syscall_handler(int eax, int ebx, int ecx, int edx) {
    switch (eax) {
        case SYS_EXIT:
            return sys_exit(ebx);
        case SYS_WRITE:
            return sys_write(ebx, (const char*)ecx, edx);
        default:
            term_print("Unsupported Syscall\n", 0xFF0000);
            return -1;
    }
}
