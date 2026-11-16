#include "shell.h"
#include <fcntl.h>

int execute(char* arglist[]) {
    int pipe_pos = -1;
    int input_pos = -1;
    int output_pos = -1;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0)
            pipe_pos = i;
        else if (strcmp(arglist[i], "<") == 0)
            input_pos = i;
        else if (strcmp(arglist[i], ">") == 0)
            output_pos = i;
    }

    if (pipe_pos != -1) {
        int fd[2];

        if (pipe(fd) == -1) {
            perror("pipe");
            return 1;
        }

        arglist[pipe_pos] = NULL;

        pid_t p1 = fork();

        if (p1 == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            execvp(arglist[0], arglist);
            perror("Command failed");
            exit(1);
        }

        pid_t p2 = fork();

        if (p2 == 0) {
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            execvp(arglist[pipe_pos + 1], &arglist[pipe_pos + 1]);
            perror("Command failed");
            exit(1);
        }

        close(fd[0]);
        close(fd[1]);

        waitpid(p1, NULL, 0);
        waitpid(p2, NULL, 0);
        return 0;
    }

    if (input_pos != -1) {
        int fd = open(arglist[input_pos + 1], O_RDONLY);

        if (fd < 0) {
            perror("Input file");
            return 1;
        }

        arglist[input_pos] = NULL;

        pid_t pid = fork();

        if (pid == 0) {
            dup2(fd, STDIN_FILENO);
            close(fd);
            execvp(arglist[0], arglist);
            perror("Command failed");
            exit(1);
        }

        close(fd);
        waitpid(pid, NULL, 0);
        return 0;
    }

    if (output_pos != -1) {
        int fd = open(arglist[output_pos + 1],
                      O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            perror("Output file");
            return 1;
        }

        arglist[output_pos] = NULL;

        pid_t pid = fork();

        if (pid == 0) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
            execvp(arglist[0], arglist);
            perror("Command failed");
            exit(1);
        }

        close(fd);
        waitpid(pid, NULL, 0);
        return 0;
    }

    if (strcmp(arglist[0], "exit") == 0)
        exit(0);

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
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(arglist[0], arglist);
        perror("Command failed");
        exit(1);
    }

    waitpid(pid, &status, 0);
    return 0;
}
