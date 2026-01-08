#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // for chdir

#define MAX_LINE 1024
#define MAX_ARGS 64

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];

    while (1) {
        // 1. Print prompt
        printf("bash-mini$ ");
        fflush(stdout);

        // 2. Read input line
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            printf("\n");
            break;
        }

        // 3. Parse line into tokens
        int argc = 0;
        char *token = strtok(line, " \t\n");

        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[argc] = NULL;

        // If empty line
        if (argc == 0) {
            continue;
        }

        // ========================
        // 4. Internal commands
        // ========================

        // exit
        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting bash-mini\n");
            break;
        }

        // cd
        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue;  // go back to prompt
        }

        // ========================
        // 5. For now: not executing external commands yet
        // ========================

        printf("External command (not implemented yet):\n");
        for (int i = 0; i < argc; i++) {
            printf("  args[%d] = '%s'\n", i, args[i]);
        }
    }

    return 0;
}

