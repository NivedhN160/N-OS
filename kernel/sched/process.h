#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../arch/isr.h"

#define MAX_PROCESSES 64

#define PROCESS_READY   0
#define PROCESS_RUNNING 1
#define PROCESS_BLOCKED 2
#define PROCESS_ZOMBIE  3

typedef struct process {
    uint32_t pid;
    uint32_t *page_directory;   // own CR3
    uint32_t kernel_stack;
    registers_t regs;
    char name[32];
    int state;
} process_t;

void process_init(void);
int process_create(const char *name, void (*entry)(void));
void process_exit(int code);
void context_switch(process_t *next);

extern process_t *current_process;

#endif
