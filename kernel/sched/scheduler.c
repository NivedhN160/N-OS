#include "scheduler.h"
#include "process.h"

extern process_t processes[MAX_PROCESSES];
extern process_t *current_process;

void scheduler_init(void) {
}

void scheduler_tick(void) {
    if (!current_process) return; // not ready

    int current_idx = -1;
    for(int i=0; i<MAX_PROCESSES; i++) {
        if(&processes[i] == current_process) {
            current_idx = i;
            break;
        }
    }

    // Round Robin
    for(int i=1; i<=MAX_PROCESSES; i++) {
        int idx = (current_idx + i) % MAX_PROCESSES;
        if (processes[idx].state == PROCESS_READY || processes[idx].state == PROCESS_RUNNING) {
            if (current_process->state == PROCESS_RUNNING) {
                current_process->state = PROCESS_READY;
            }
            processes[idx].state = PROCESS_RUNNING;
            context_switch(&processes[idx]);
            return;
        }
    }
}
