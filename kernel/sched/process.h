#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 16

typedef void (*process_func_t)();

typedef struct {
    int id;
    int active;
    char name[32];
    process_func_t func;
} Process;

extern Process processes[MAX_PROCESSES];

void process_init();
int process_create(const char *name, process_func_t func);
void process_kill(int id);
void process_schedule();

#endif
