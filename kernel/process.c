#include "process.h"

Process processes[MAX_PROCESSES];

static void scopy(char *d, const char *s){
    int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;
}

void process_init() {
    for (int i=0; i<MAX_PROCESSES; i++) {
        processes[i].active = 0;
    }
}

int process_create(const char *name, process_func_t func) {
    for (int i=0; i<MAX_PROCESSES; i++) {
        if (!processes[i].active) {
            processes[i].id = i;
            scopy(processes[i].name, name);
            processes[i].func = func;
            processes[i].active = 1;
            return i;
        }
    }
    return -1;
}

void process_kill(int id) {
    if (id >= 0 && id < MAX_PROCESSES) {
        processes[id].active = 0;
    }
}

void process_schedule() {
    for (int i=0; i<MAX_PROCESSES; i++) {
        if (processes[i].active && processes[i].func) {
            processes[i].func();
        }
    }
}
