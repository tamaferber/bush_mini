#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // fork, execv, chdir
#include <sys/wait.h>   // wait
#include <sys/stat.h>   // stat

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_PATH 1024

// Check if file exists and is executable
int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0)
        return 0;
    if (!S_ISREG(st.st_mode))
        return 0;
    if (st.st_mode & S_IXUSR)
        return 1;
    return 0;
}

// Try to find command in HOME or /bin
// Returns 1 if found and fills fullpath, else 0
int find_command(const char *cmd, char *fullpath) {
    char *home = getenv("HOME");

    if (home != NULL) {
        snprintf(fullpath, MAX_PATH, "%s/%s", home, cmd);
        if (is_executable(fullpath)) {
            return 1;
        }
    }

    // Try /bin
    snprintf(fullpath, MAX_PATH, "/bin/%s", cmd);
    if (is_executable(fullpath)) {
        return 1;
    }

    return 0;
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];

    while (1) {
        // 1. Prompt
        printf("bash-mini$ ");
        fflush(stdout);

        // 2. Read input
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            printf("\n");
            break;
        }

        // 3. Parse
        int argc = 0;
        char *token = strtok(line, " \t\n");
        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[argc] = NULL;

        if (argc == 0)
            continue;

        // ========================
        // Internal commands
        // ========================

        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting bash-mini...\n");
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        // ========================
        // External commands
        // ========================

        char fullpath[MAX_PATH];

        if (!find_command(args[0], fullpath)) {
            printf("[%s]: Unknown Command\n", args[0]);
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // Child
            execv(fullpath, args);

            // If execv returns, it's an error
            perror("execv");
            exit(1);
        } else {
            // Parent
            int status;
            wait(&status);

            if (WIFEXITED(status)) {
                int code = WEXITSTATUS(status);
                printf("Command finished. Return code = %d\n", code);
            } else {
                printf("Command terminated abnormally\n");
            }
        }
    }

    return 0;
}

