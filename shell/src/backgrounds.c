#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "backgrounds.h"

background_info backgrounds_array[MAX_BACKGROUNDS];
status_info status_array[MAX_STATUSES];


int foregrounds_count;
int backgrounds_count;
int status_count;


void
write_statuses() {

    for (int i = 0; i < status_count; i++) {
        if (status_array[i].killedbysignal == 1) {
            fprintf(stdout, "Background process %d terminated. (killed by signal %d)\n", status_array[i].pid, status_array[i].signal);
        }
        else {
            fprintf(stdout, "Background process %d terminated. (exited with status %d)\n", status_array[i].pid, status_array[i].status);
        }
    }

    status_count = 0;
}


void
add_background(pid_t pid) {

    for (int i = 0; i < MAX_BACKGROUNDS; i++) { //we finish loop when not active so it does no take that long
        if (backgrounds_array[i].active != 1) {
            backgrounds_array[i].pid = pid;
            backgrounds_array[i].active = 1;
            return;
        }

    }
}

void
add_status(int pid, int status) {
    if (status_count >= MAX_STATUSES) {
        return;
    }

    status_array[status_count].pid = pid;

    if (WIFEXITED(status)) {
        status_array[status_count].status = WEXITSTATUS(status);
        status_array[status_count].killedbysignal = 0;
    }
    else if (WIFSIGNALED(status)) {
        status_array[status_count].signal = WTERMSIG(status);
        status_array[status_count].killedbysignal = 1;
    }
    status_count++;
}

void
child_handler(int signo) {
    int status;
    int is_background = 0;
    while (1) {
        pid_t ch = waitpid(-1, &status, WNOHANG);
        if (ch <= 0) break;
        for (int i = 0; i < MAX_BACKGROUNDS; i++) {
            if (backgrounds_array[i].pid == ch) {
                is_background = 1;
                backgrounds_array[i].active = 0;
                break;
            }
        }
        if (is_background) {
            add_status(ch, status);
        }
        else { //is_foreground
            foregrounds_count--;
        }
    }
}