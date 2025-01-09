#ifndef BACKGROUNDS_H
#define BACKGROUNDS_H


#include <unistd.h>

#define MAX_BACKGROUNDS 2048
#define MAX_STATUSES 2048

extern int backgrounds_count;
extern int foregrounds_count;
extern int status_count;


typedef struct background_info {
    pid_t pid;
    int active;
} background_info;


typedef struct status_info {
    pid_t pid;
    int status;
    int signal;
    int killedbysignal;
} status_info;

void add_background(pid_t);
void child_handler(int);
void write_statuses();
void add_status(pid_t, int);

#endif //BACKGROUNDS_H
