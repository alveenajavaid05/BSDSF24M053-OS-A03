#include "shell.h"
#include <fcntl.h>

pid_t jobs_list[100];
int job_count = 0;

void clean_jobs()
{
    for (int i = 0; i < job_count; ) {
        int status;
        pid_t result = waitpid(jobs_list[i], &status, WNOHANG);

        if (result > 0 || (result == -1 && errno == ECHILD)) {
            for (int j = i; j < job_count - 1; j++)
                jobs_list[j] = jobs_list[j + 1];

            job_count--;
        } else {
            i++;
        }
    }
}

int execute_single(char *arglist[])
{
    int pipe_pos = -1;
    int input_pos = -1;
    int output_pos = -1;

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
        printf("exit - Exit shell\n");
        printf("cd - Change directory\n");
        printf("help - Display help\n");
        printf("jobs - Display background jobs\n");
        return 0;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        clean_jobs();

        if (job_count == 0)
            printf("No background jobs.\n");

        for (int i = 0; i < job_count; i++)
            printf("[%d] Running PID: %d\n", i + 1, jobs_list[i]);

        return 0;
    }

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

    waitpid(pid, NULL, 0);
    return 0;
}

int execute_condition(char *arglist[])
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(arglist[0], arglist);
        perror("Condition failed");
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;
}

int execute(char *arglist[])
{
    clean_jobs();

    char *segment[MAXARGS + 1];
    int count = 0;

    for (int i = 0; ; i++) {
        if (arglist[i] == NULL ||
            strcmp(arglist[i], ";") == 0 ||
            strcmp(arglist[i], "&") == 0) {

            segment[count] = NULL;

            if (count > 0) {
                if (arglist[i] != NULL &&
                    strcmp(arglist[i], "&") == 0) {

                    if (job_count < 100) {
                        pid_t pid = fork();

                        if (pid == 0) {
                            execute_single(segment);
                            exit(0);
                        }

                        if (pid > 0) {
                            jobs_list[job_count++] = pid;
                            printf("[%d] Started PID: %d\n",
                                   job_count, pid);
                        } else {
                            perror("fork");
                        }
                    } else {
                        printf("Job list is full.\n");
                    }
                } else {
                    execute_single(segment);
                }
            }

            count = 0;

            if (arglist[i] == NULL)
                break;
        } else {
            if (count < MAXARGS)
                segment[count++] = arglist[i];
        }
    }

    clean_jobs();
    return 0;
}
