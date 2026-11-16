#include "shell.h"

// Helper function to execute a command inside a child process with redirection (< and >)
static void execute_child_with_redirection(char* arglist[]) {
    char* input_file = NULL;
    char* output_file = NULL;
    char* clean_args[MAXARGS + 1];
    int clean_idx = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            if (arglist[i + 1] != NULL) {
                input_file = arglist[i + 1];
                i++; // Skip filename
            }
        } else if (strcmp(arglist[i], ">") == 0) {
            if (arglist[i + 1] != NULL) {
                output_file = arglist[i + 1];
                i++; // Skip filename
            }
        } else {
            clean_args[clean_idx++] = arglist[i];
        }
    }
    clean_args[clean_idx] = NULL;

    if (clean_idx == 0) {
        exit(0);
    }

    // Input redirection (<)
    if (input_file != NULL) {
        int in_fd = open(input_file, O_RDONLY);
        if (in_fd < 0) {
            perror(input_file);
            exit(1);
        }
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }

    // Output redirection (>)
    if (output_file != NULL) {
        int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            perror(output_file);
            exit(1);
        }
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    execvp(clean_args[0], clean_args);
    perror("Command not found");
    exit(1);
}

// Function to handle pipes (|)
int execute_pipe(char* left_cmd[], char* right_cmd[], int is_bg, char* cmd_str) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe failed");
        return -1;
    }

    // Left child (Writer): standard output redirected to pipe's write end
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execute_child_with_redirection(left_cmd);
    }

    // Right child (Reader): standard input redirected to pipe's read end
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execute_child_with_redirection(right_cmd);
    }

    // Parent closes both pipe ends to avoid hangs
    close(fd[0]);
    close(fd[1]);

    if (is_bg) {
        add_job(pid2, cmd_str);
    } else {
        // Wait for both children to complete
        int status;
        waitpid(pid1, &status, 0);
        waitpid(pid2, &status, 0);
    }
    return 0;
}

int execute(char* arglist[], int is_bg, char* cmd_str) {
    // 1. Check if pipe (|) is present
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            char* left_cmd[MAXARGS + 1];
            char* right_cmd[MAXARGS + 1];

            for (int j = 0; j < i; j++) {
                left_cmd[j] = arglist[j];
            }
            left_cmd[i] = NULL;

            int k = 0;
            for (int j = i + 1; arglist[j] != NULL; j++) {
                right_cmd[k++] = arglist[j];
            }
            right_cmd[k] = NULL;

            return execute_pipe(left_cmd, right_cmd, is_bg, cmd_str);
        }
    }

    // 2. Single command (with or without redirection)
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0:
            execute_child_with_redirection(arglist);
            break;
        default:
            if (is_bg) {
                add_job(cpid, cmd_str);
            } else {
                waitpid(cpid, &status, 0);
            }
            return 0;
    }
    return 0;
}