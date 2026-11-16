#include "shell.h"

int main() {
    char* cmdline;

    init_readline();

    while (1) {
        check_zombies();

        cmdline = readline(PROMPT);
        if (cmdline == NULL) {
            break; // Handle Ctrl+D / EOF
        }

        // Skip empty input
        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        // Handle !n re-execution before tokenization or adding to history
        if (cmdline[0] == '!') {
            int n = atoi(&cmdline[1]);
            char* past_cmd = get_history(n);
            if (past_cmd == NULL) {
                printf("!%s: event not found\n", &cmdline[1]);
                free(cmdline);
                continue;
            }
            printf("%s\n", past_cmd);
            free(cmdline);
            cmdline = strdup(past_cmd);
        }

        // Add to history (GNU Readline + custom storage)
        add_history(cmdline);
        add_to_history(cmdline);

        // Split multiple commands separated by semicolon (;)
        char* saveptr;
        char* subcmd = strtok_r(cmdline, ";", &saveptr);

        while (subcmd != NULL) {
            // Trim leading whitespace
            while (*subcmd == ' ' || *subcmd == '\t') subcmd++;

            if (*subcmd != '\0') {
                char job_name[MAX_LEN];
                strncpy(job_name, subcmd, MAX_LEN - 1);
                job_name[MAX_LEN - 1] = '\0';

                char** arglist = tokenize(subcmd);
                if (arglist != NULL) {
                    // Check for background execution (&)
                    int is_bg = 0;
                    int last = 0;
                    while (arglist[last] != NULL) last++;

                    if (last > 0 && strcmp(arglist[last - 1], "&") == 0) {
                        is_bg = 1;
                        free(arglist[last - 1]);
                        arglist[last - 1] = NULL;
                    }

                    if (arglist[0] != NULL) {
                        if (handle_builtin(arglist) == 0) {
                            execute(arglist, is_bg, job_name);
                        }
                    }

                    // Free memory allocated by tokenize()
                    for (int i = 0; arglist[i] != NULL; i++) {
                        free(arglist[i]);
                    }
                    free(arglist);
                }
            }

            subcmd = strtok_r(NULL, ";", &saveptr);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}