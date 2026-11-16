#include "shell.h"

int execute(char* arglist[]) {

    if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        char *directory = arglist[1];

        if (directory == NULL)
            directory = getenv("HOME");

        if (chdir(directory) != 0)
            perror("cd");

        return 0;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("exit - Exit the shell\n");
        printf("cd - Change directory\n");
        printf("help - Display help\n");
        printf("jobs - Display background jobs\n");
        return 0;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        printf("No background jobs.\n");
        return 0;
    }

    int status;
    int cpid = fork();

    if (cpid == -1) {
        perror("fork failed");
        return 1;
    }

    if (cpid == 0) {
        execvp(arglist[0], arglist);
        perror("Command not found");
        exit(1);
    }

    waitpid(cpid, &status, 0);
    return 0;
}
