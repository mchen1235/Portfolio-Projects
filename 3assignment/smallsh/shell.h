#ifndef SHELL_H
#define SHELL_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
int allow_background;

//struct to hold a command input
struct Command{
    int comment;
    int argument_count;
    char* arguments[512];
    char* input_file;
    char* output_file;
    int background;
    char* command;
};

//struct to hold the process id of background processes in a linked list
struct Bg_pid{
    pid_t pid;
    struct Bg_pid* next;
};

struct Command* parse_string();
void clean_struct(struct Command*);
void execute(struct Command*, int*, int*, struct Bg_pid**, struct sigaction*, struct sigaction*);
void redirection(char*, int, int*);
void clean_bg(struct Bg_pid**, int*);
void child_fork(struct Command*, int*, struct sigaction*, struct sigaction*);
void parent_fork(struct Command*, int*, pid_t*, int*, struct Bg_pid**, struct sigaction*);
void handle_SIGINT(int);
void handle_SIGTSTP(int);

#endif
