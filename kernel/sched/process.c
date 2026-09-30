#include "process.h"
#include "../mm/kmalloc.h"

extern void term_print(const char *s, unsigned int col);

process_t processes[MAX_PROCESSES];
process_t *current_process = 0;
static uint32_t next_pid = 1;

static void scopy(char *d, const char *s){
    int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;
}

void process_init(void) {
    for (int i=0; i<MAX_PROCESSES; i++) {
        processes[i].state = PROCESS_ZOMBIE;
    }
    term_print("Process Manager Initialized.\n", 0x00FF00);
}

int process_create(const char *name, void (*entry)(void)) {
    for (int i=0; i<MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_ZOMBIE) {
            processes[i].pid = next_pid++;
            scopy(processes[i].name, name);
            processes[i].state = PROCESS_READY;
            processes[i].regs.eip = (uint32_t)entry;
            // set up stack, etc.
            return processes[i].pid;
        }
    }
    return -1;
}

void process_exit(int code) {
    if (current_process) {
        current_process->state = PROCESS_ZOMBIE;
    }
    while(1) { __asm__ volatile("hlt"); } // Wait for scheduler
}

void context_switch(process_t *next) {
    // Basic stub. Real context switch needs assembly.
    current_process = next;
}
