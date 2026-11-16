#include "shell.h"

#define HISTORY_SIZE 20
#define BLOCK_SIZE 20

char history[HISTORY_SIZE][MAX_LEN];
int history_numbers[HISTORY_SIZE];
int history_count = 0;
int total_commands = 0;

void save_history(char *cmd)
{
    if (history_count == HISTORY_SIZE) {
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            strcpy(history[i], history[i + 1]);
            history_numbers[i] = history_numbers[i + 1];
        }
        history_count--;
    }

    strcpy(history[history_count], cmd);
    total_commands++;
    history_numbers[history_count] = total_commands;
    history_count++;
}

void show_history()
{
    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", history_numbers[i], history[i]);
    }
}

void run_block(char block[][MAX_LEN], int count)
{
    for (int i = 0; i < count; i++) {
        char **args = tokenize(block[i]);

        if (args != NULL) {
            execute(args);

            for (int j = 0; args[j] != NULL; j++)
                free(args[j]);

            free(args);
        }
    }
}

void handle_if(char *cmdline)
{
    char then_block[BLOCK_SIZE][MAX_LEN];
    char else_block[BLOCK_SIZE][MAX_LEN];

    int then_count = 0;
    int else_count = 0;
    int in_else = 0;
    int found_then = 0;
    int found_fi = 0;

    char **condition = tokenize(cmdline + 2);

    if (condition == NULL) {
        printf("Invalid if condition.\n");
        return;
    }

    int status = execute_condition(condition);

    for (int i = 0; condition[i] != NULL; i++)
        free(condition[i]);

    free(condition);

    char *line;

    while ((line = read_cmd("if> ", stdin)) != NULL) {

        if (strcmp(line, "then") == 0) {
            found_then = 1;
        }
        else if (strcmp(line, "else") == 0) {
            in_else = 1;
        }
        else if (strcmp(line, "fi") == 0) {
            found_fi = 1;
            free(line);
            break;
        }
        else if (found_then) {
            if (!in_else && then_count < BLOCK_SIZE) {
                strcpy(then_block[then_count++], line);
            }
            else if (in_else && else_count < BLOCK_SIZE) {
                strcpy(else_block[else_count++], line);
            }
        }

        free(line);
    }

    if (!found_then || !found_fi) {
        printf("Invalid if block. Use then, else and fi.\n");
        return;
    }

    if (status == 0) {
        run_block(then_block, then_count);
    }
    else {
        run_block(else_block, else_count);
    }
}

int main()
{
    char *cmdline;
    char **arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        if (cmdline[0] == '!' &&
            cmdline[1] >= '0' && cmdline[1] <= '9') {

            int number = atoi(cmdline + 1);
            int found = 0;

            for (int i = 0; i < history_count; i++) {
                if (history_numbers[i] == number) {
                    strcpy(cmdline, history[i]);
                    found = 1;
                    printf("%s\n", cmdline);
                    break;
                }
            }

            if (!found) {
                printf("Command not found in history.\n");
                free(cmdline);
                continue;
            }
        }

        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        save_history(cmdline);

        if (strcmp(cmdline, "history") == 0) {
            show_history();
            free(cmdline);
            continue;
        }

        if (strncmp(cmdline, "if ", 3) == 0) {
            handle_if(cmdline);
            free(cmdline);
            continue;
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            execute(arglist);

            for (int i = 0; arglist[i] != NULL; i++)
                free(arglist[i]);

            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
