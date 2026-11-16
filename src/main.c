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

        // Check for if-then-else-fi statement
        if (strncmp(cmdline, "if ", 3) == 0 || strcmp(cmdline, "if") == 0) {
            handle_if_block(cmdline);
            free(cmdline);
            continue;
        }

        // Split multiple commands separated by semicolon (;)
        char* saveptr;
        char* subcmd = strtok_r(cmdline, ";", &saveptr);

        while (subcmd != NULL) {
            // Trim leading whitespace
            while (*subcmd == ' ' || *subcmd == '\t') subcmd++;

            if (*subcmd != '\0') {
                run_single_command(subcmd);
            }

            subcmd = strtok_r(NULL, ";", &saveptr);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}