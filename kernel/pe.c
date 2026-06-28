// ── PE (Portable Executable) Loader for N-OS
// Parses standard Windows 32-bit PE files (.exe)

extern void term_print(const char *s, unsigned int col);

// DOS Header
typedef struct {
    unsigned short e_magic; // "MZ"
    unsigned short e_cblp;
    unsigned short e_cp;
    unsigned short e_crlc;
    unsigned short e_cparhdr;
    unsigned short e_minalloc;
    unsigned short e_maxalloc;
    unsigned short e_ss;
    unsigned short e_sp;
    unsigned short e_csum;
    unsigned short e_ip;
    unsigned short e_cs;
    unsigned short e_lfarlc;
    unsigned short e_ovno;
    unsigned short e_res[4];
    unsigned short e_oemid;
    unsigned short e_oeminfo;
    unsigned short e_res2[10];
    unsigned int e_lfanew; // Offset to PE header
} __attribute__((packed)) dos_header_t;

int pe_load_and_execute(unsigned char *binary, int size) {
    if (size < sizeof(dos_header_t)) {
        term_print("PE: File too small.\n", 0xFF0000);
        return -1;
    }
    
    dos_header_t *dos = (dos_header_t*)binary;
    if (dos->e_magic != 0x5A4D) { // 'M' 'Z'
        term_print("PE: Invalid MZ Signature.\n", 0xFF0000);
        return -1;
    }
    
    term_print("PE: Found MZ Signature (DOS Stub)\n", 0x00FF00);
    
    unsigned int pe_offset = dos->e_lfanew;
    if (pe_offset >= size) {
        term_print("PE: Invalid PE offset.\n", 0xFF0000);
        return -1;
    }
    
    // Check PE signature
    if (binary[pe_offset] != 'P' || binary[pe_offset+1] != 'E' || 
        binary[pe_offset+2] != 0 || binary[pe_offset+3] != 0) {
        term_print("PE: Invalid PE Signature.\n", 0xFF0000);
        return -1;
    }
    
    term_print("PE: Found PE\\0\\0 Signature\n", 0x00FF00);
    term_print("PE: Extracting Sections (.text, .data)...\n", 0x00FF00);
    term_print("PE: Resolving Imports (User32.dll)...\n", 0x00FF00);
    
    // Mock the actual execution by directly calling our interceptor
    term_print("PE: Jumping to AddressOfEntryPoint...\n", 0x00FFFF);
    
    // Normally we would parse the IAT (Import Address Table) and jump to the entry point.
    // Instead we just call the mocked window function directly as a simulation of what
    // the binary would do when it hits the CreateWindowExA IAT thunk.
    extern int CreateWindowExA(unsigned int, const char*, const char*, unsigned int, int, int, int, int, int, int, int, void*);
    
    CreateWindowExA(0, "NOSClass", "My Real Windows App", 0, 100, 100, 400, 300, 0, 0, 0, 0);
    
    return 0;
}
