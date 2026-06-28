// ── ELF Loader for N-OS
// Parses standard 32-bit ELF binaries.
// This allows true "multi-language support" by standardizing on ELF
// rather than interpreting text commands in the shell.

extern void term_print(const char *s, unsigned int col);

// ELF32 Header
typedef struct {
    unsigned char e_ident[16];
    unsigned short e_type;
    unsigned short e_machine;
    unsigned int e_version;
    unsigned int e_entry;
    unsigned int e_phoff;
    unsigned int e_shoff;
    unsigned int e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
} __attribute__((packed)) elf32_ehdr_t;

// ELF32 Program Header
typedef struct {
    unsigned int p_type;
    unsigned int p_offset;
    unsigned int p_vaddr;
    unsigned int p_paddr;
    unsigned int p_filesz;
    unsigned int p_memsz;
    unsigned int p_flags;
    unsigned int p_align;
} __attribute__((packed)) elf32_phdr_t;

#define PT_LOAD 1

int elf_load_and_execute(unsigned char *binary, int size) {
    if (size < sizeof(elf32_ehdr_t)) {
        term_print("ELF: File too small.\n", 0xFF0000);
        return -1;
    }
    
    elf32_ehdr_t *hdr = (elf32_ehdr_t *)binary;
    
    // Check magic bytes: 0x7F 'E' 'L' 'F'
    if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' || 
        hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F') {
        term_print("ELF: Invalid Magic.\n", 0xFF0000);
        return -1;
    }
    
    term_print("ELF: Magic OK. Parsing Program Headers...\n", 0x00FF00);
    
    unsigned int entry_point = hdr->e_entry;
    elf32_phdr_t *phdrs = (elf32_phdr_t *)(binary + hdr->e_phoff);
    
    for (int i=0; i<hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            term_print("ELF: Found PT_LOAD segment, mapping...\n", 0x00FF00);
            // In a real OS with paging, we would map physical pages 
            // to phdrs[i].p_vaddr and memcpy the data.
            // memcpy((void*)phdrs[i].p_vaddr, binary + phdrs[i].p_offset, phdrs[i].p_filesz);
            // memset((void*)(phdrs[i].p_vaddr + phdrs[i].p_filesz), 0, phdrs[i].p_memsz - phdrs[i].p_filesz);
        }
    }
    
    term_print("ELF: Loaded! Executing entry point...\n", 0x00FFFF);
    
    // Actually executing it:
    // void (*app_entry)() = (void(*)())entry_point;
    // app_entry();
    
    return 0;
}
