#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 20
#define ARGLEN 30
#define HISTORY_SIZE 20
#define MAX_JOBS 20
#define PROMPT "FCIT> "

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist, int is_bg, char* cmd_str);
int handle_builtin(char** arglist);
void add_to_history(char* cmd);
void print_history(void);
char* get_history(int n);
void init_readline(void);
void add_job(pid_t pid, char* cmd);
void remove_job(pid_t pid);
void check_zombies(void);
void print_jobs(void);
int run_single_command(char* cmd);
void handle_if_block(char* cmdline);

#endif // SHELL_H