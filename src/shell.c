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
        // Recognize special operators: <, >, |, and &
        if (*cp == '<' || *cp == '>' || *cp == '|' || *cp == '&') {
            len = 1;
            cp++;
        } else if (*cp == '"' || *cp == '\'') {
            char quote = *cp++;
            start = cp;
            len = 0;
            while (*cp != '\0' && *cp != quote) {
                len++;
                cp++;
            }
            if (*cp == quote) cp++; // Skip closing quote
        } else {
            len = 0;
            while (*cp != '\0' && !(*cp == ' ' || *cp == '\t' || *cp == '<' || *cp == '>' || *cp == '|' || *cp == '&')) {
                len++;
                cp++;
            }
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

// Background jobs management
typedef struct {
    pid_t pid;
    char cmd[MAX_LEN];
} Job;

static Job bg_jobs[MAX_JOBS];
static int job_count = 0;

void add_job(pid_t pid, char* cmd) {
    if (job_count < MAX_JOBS) {
        bg_jobs[job_count].pid = pid;
        strncpy(bg_jobs[job_count].cmd, cmd, MAX_LEN - 1);
        bg_jobs[job_count].cmd[MAX_LEN - 1] = '\0';
        printf("[%d] %d\n", job_count + 1, pid);
        job_count++;
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (bg_jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                bg_jobs[j] = bg_jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void check_zombies(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

void print_jobs(void) {
    check_zombies();
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s\n", i + 1, bg_jobs[i].pid, bg_jobs[i].cmd);
    }
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

    // jobs: lists running background jobs
    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }

    return 0;
}

// Built-in command names for Tab completion
static char* builtin_names[] = {
    "cd",
    "exit",
    "help",
    "history",
    "jobs",
    NULL
};

// Generator function for command completion
static char* command_generator(const char* text, int state) {
    static int list_index, len;
    char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = builtin_names[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

// Custom completion handler
static char** shell_completion(const char* text, int start, int end) {
    (void)end;
    rl_attempted_completion_over = 0;

    // At start of line, complete command names
    if (start == 0) {
        char** matches = rl_completion_matches(text, command_generator);
        if (matches != NULL) {
            return matches;
        }
    }

    // Default: filename completion
    return NULL;
}

void init_readline(void) {
    rl_bind_key('\t', rl_complete);
    rl_attempted_completion_function = shell_completion;
}