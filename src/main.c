
#include "shell.h"
#include <ctype.h>

#define HISTORY_SIZE 20
#define BLOCK_SIZE 20
#define VARIABLE_COUNT 50
#define NAME_SIZE 50

char history[HISTORY_SIZE][MAX_LEN];
int history_numbers[HISTORY_SIZE];
int history_count = 0;
int total_commands = 0;

struct ShellVariable {
    char name[NAME_SIZE];
    char value[MAX_LEN];
};

struct ShellVariable variables[VARIABLE_COUNT];
int variable_count = 0;

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
    for (int i = 0; i < history_count; i++)
        printf("%d  %s\n", history_numbers[i], history[i]);
}

void set_variable(char *name, char *value)
{
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            snprintf(variables[i].value, MAX_LEN, "%s", value);
            return;
        }
    }

    if (variable_count < VARIABLE_COUNT) {
        snprintf(variables[variable_count].name, NAME_SIZE, "%s", name);
        snprintf(variables[variable_count].value, MAX_LEN, "%s", value);
        variable_count++;
    } else {
        printf("Variable limit reached.\n");
    }
}

char *get_variable(char *name)
{
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0)
            return variables[i].value;
    }

    return "";
}

void show_variables()
{
    for (int i = 0; i < variable_count; i++)
        printf("%s=%s\n", variables[i].name, variables[i].value);
}

int handle_assignment(char *cmd)
{
    char *equal = strchr(cmd, '=');

    if (equal == NULL)
        return 0;

    int length = equal - cmd;

    if (length <= 0 || length >= NAME_SIZE)
        return 0;

    if (!isalpha((unsigned char)cmd[0]) && cmd[0] != '_')
        return 0;

    for (int i = 0; i < length; i++) {
        if (!isalnum((unsigned char)cmd[i]) && cmd[i] != '_')
            return 0;
    }

    char name[NAME_SIZE];

    strncpy(name, cmd, length);
    name[length] = '\0';

    set_variable(name, equal + 1);
    return 1;
}

void expand_variables(char *input, char *output, int size)
{
    int i = 0;
    int j = 0;

    while (input[i] != '\0' && j < size - 1) {

        if (input[i] == '$' &&
            (isalpha((unsigned char)input[i + 1]) ||
             input[i + 1] == '_')) {

            char name[NAME_SIZE];
            int k = 0;
            i++;

            while ((isalnum((unsigned char)input[i]) ||
                    input[i] == '_') && k < NAME_SIZE - 1) {
                name[k++] = input[i++];
            }

            name[k] = '\0';

            char *value = get_variable(name);

            while (*value != '\0' && j < size - 1)
                output[j++] = *value++;
        }
        else {
            output[j++] = input[i++];
        }
    }

    output[j] = '\0';
}

void run_block(char block[][MAX_LEN], int count)
{
    for (int i = 0; i < count; i++) {
        char expanded[MAX_LEN];
        expand_variables(block[i], expanded, MAX_LEN);

        if (handle_assignment(expanded))
            continue;

        if (strcmp(expanded, "set") == 0) {
            show_variables();
            continue;
        }

        char **args = tokenize(expanded);

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
            if (!in_else && then_count < BLOCK_SIZE)
                strcpy(then_block[then_count++], line);
            else if (in_else && else_count < BLOCK_SIZE)
                strcpy(else_block[else_count++], line);
        }

        free(line);
    }

    if (!found_then || !found_fi) {
        printf("Invalid if block. Use then, else and fi.\n");
        return;
    }

    if (status == 0)
        run_block(then_block, then_count);
    else
        run_block(else_block, else_count);
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

        if (strcmp(cmdline, "set") == 0) {
            show_variables();
            free(cmdline);
            continue;
        }

        char expanded[MAX_LEN];
        expand_variables(cmdline, expanded, MAX_LEN);

        if (handle_assignment(expanded)) {
            free(cmdline);
            continue;
        }

        if (strncmp(expanded, "if ", 3) == 0) {
            handle_if(expanded);
            free(cmdline);
            continue;
        }

        if ((arglist = tokenize(expanded)) != NULL) {
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

