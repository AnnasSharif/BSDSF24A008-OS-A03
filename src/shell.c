#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace
        
        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

// History storage
static char* history[HISTORY_SIZE];
static int history_count = 0;

void add_to_history(char* cmd) {
    if (cmd == NULL || cmd[0] == '\0') {
        return;
    }

    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmd);
        history_count++;
    } else {
        // History full: remove oldest, shift left, add newest at end
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

void print_history(void) {
    for (int i = 0; i < history_count; i++) {
        printf("%4d  %s\n", i + 1, history[i]);
    }
}

char* get_history(int n) {
    if (n < 1 || n > history_count) {
        return NULL;
    }
    return history[n - 1];
}

int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    // exit: terminates the shell gracefully
    if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    }

    // cd <directory>: changes current directory
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            char* home = getenv("HOME");
            if (home != NULL) {
                chdir(home);
            }
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }

    // history: displays command history
    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    // help: displays list of built-in commands
    if (strcmp(arglist[0], "help") == 0) {
        printf("Available built-in commands:\n");
        printf("  cd <path> : Change directory\n");
        printf("  exit      : Exit the shell\n");
        printf("  help      : Show this help message\n");
        printf("  history   : Show command history\n");
        printf("  jobs      : List background jobs\n");
        return 1;
    }

    // jobs: placeholder for job control
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0;
}