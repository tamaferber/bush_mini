#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // fork, execv, chdir
#include <sys/wait.h>   // wait
#include <sys/stat.h>   // stat

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_PATH 1024

/*
 * Check if a given path points to a regular file
 * and if that file has executable permission.
 *
 * Uses stat() system call to retrieve file metadata.
 * Returns 1 if executable, 0 otherwise.
 */
int is_executable(const char *path) {
    struct stat st;

    // stat() fills 'st' with information about the file
    if (stat(path, &st) != 0)
        return 0;

    // Check that it is a regular file (not a directory, etc.)
    if (!S_ISREG(st.st_mode))
        return 0;

    // Check that the user-executable bit is set
    if (st.st_mode & S_IXUSR)
        return 1;

    return 0;
}

/*
 * Try to locate the command in:
 * 1) The user's HOME directory
 * 2) The /bin directory
 *
 * If found, writes the full path into 'fullpath' and returns 1.
 * Otherwise returns 0.
 *
 * Uses getenv() to get HOME and stat() to test file existence and permissions.
 */
int find_command(const char *cmd, char *fullpath) {
    char *home = getenv("HOME");

    // First: try HOME directory
    if (home != NULL) {
        snprintf(fullpath, MAX_PATH, "%s/%s", home, cmd);
        if (is_executable(fullpath)) {
            return 1;
        }
    }

    // Second: try /bin
    snprintf(fullpath, MAX_PATH, "/bin/%s", cmd);
    if (is_executable(fullpath)) {
        return 1;
    }

    // Not found anywhere
    return 0;
}

int main() {
    char line[MAX_LINE];      // Buffer for user input line
    char *args[MAX_ARGS];     // Array of pointers to parsed arguments

    while (1) {
        // ========================
        // 1. Print prompt
        // ========================
        printf("bash-mini$ ");
        fflush(stdout);  // Force the prompt to appear immediately

        // ========================
        // 2. Read input line
        // ========================
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            // EOF (Ctrl+D) or input error
            printf("\n");
            break;
        }

        // ========================
        // 3. Parse input into tokens
        // ========================
        int argc = 0;
        char *token = strtok(line, " \t\n");

        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[argc] = NULL;  // execv requires NULL-terminated array

        // If user just pressed Enter, do nothing
        if (argc == 0)
            continue;

        // ========================
        // 4. Internal commands
        // ========================

        // exit: terminate the shell itself
        if (strcmp(args[0], "exit") == 0) {
            printf("Exiting bash-mini...\n");
            break;
        }

        // cd: change current working directory of THIS process
        // Must be internal, because only the shell process can change its own directory
        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                // chdir() is a system call that changes the process working directory
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue; // Return to prompt
        }

        // ========================
        // 5. External commands
        // ========================

        char fullpath[MAX_PATH];

        // Try to find the command in HOME or /bin
        if (!find_command(args[0], fullpath)) {
            printf("[%s]: Unknown Command\n", args[0]);
            continue;
        }

        // ========================
        // 6. Process creation
        // ========================

        // fork() creates a new child process which is a copy of this process
        pid_t pid = fork();

        if (pid < 0) {
            // fork failed
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // ========================
            // Child process
            // ========================

            // execv() replaces the current process image with a new program
            // If execv returns, it means an error occurred
            execv(fullpath, args);

            // Only reached if execv failed
            perror("execv");
            exit(1);
        } else {
            // ========================
            // Parent process (the shell)
            // ========================

            int status;

            // wait() blocks the parent until the child process terminates
            wait(&status);

            // Check how the child terminated
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

