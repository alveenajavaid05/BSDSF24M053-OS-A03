#include "shell.h"

#define HISTORY_SIZE 20

char history[HISTORY_SIZE][MAX_LEN];
int history_numbers[HISTORY_SIZE];
int history_count = 0;
int total_commands = 0;

void add_history(char *cmd)
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

int main()
{
    char *cmdline;
    char **arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        if (cmdline[0] == '!' && cmdline[1] >= '0' && cmdline[1] <= '9') {
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

        add_history(cmdline);

        if (strcmp(cmdline, "history") == 0) {
            show_history();
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
