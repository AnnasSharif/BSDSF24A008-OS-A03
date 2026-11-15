#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
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

        // Add to history
        add_to_history(cmdline);

        if ((arglist = tokenize(cmdline)) != NULL) {
            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}