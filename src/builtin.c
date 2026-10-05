#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "builtin.h"

/* ---------------- CD ---------------- */

static int builtin_cd(command_t *cmd)
{
    const char *directory;

    if (cmd->argc == 1)
    {
        directory = getenv("HOME");

        if (directory == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }
    else if (cmd->argc == 2)
    {
        directory = cmd->argv[1];
    }
    else
    {
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }

    if (chdir(directory) != 0)
    {
        perror("cd");
        return -1;
    }

    return 0;
}

/* ---------------- PWD ---------------- */

static int builtin_pwd(command_t *cmd)
{
    char current_directory[4096];

    if (cmd->argc > 1)
    {
        fprintf(stderr, "pwd: too many arguments\n");
        return -1;
    }

    if (getcwd(current_directory, sizeof(current_directory)) == NULL)
    {
        perror("pwd");
        return -1;
    }

    printf("%s\n", current_directory);

    return 0;
}

/* ---------------- ECHO ---------------- */

static int builtin_echo(command_t *cmd)
{
    for (int i = 1; i < cmd->argc; i++)
    {
        printf("%s", cmd->argv[i]);

        if (i < cmd->argc - 1)
            printf(" ");
    }

    printf("\n");

    return 0;
}

/* ---------------- EXIT ---------------- */

static int builtin_exit(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr, "exit: too many arguments\n");
        return -1;
    }

    return 1;
}

/* ---------------- FORK ---------------- */

static int builtin_fork(command_t *cmd)
{
    pid_t pid;

    if (cmd->argc > 1)
    {
        fprintf(stderr, "fork: too many arguments\n");
        return -1;
    }

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        printf("Child process created\n");
        printf("Child PID: %d\n", getpid());
        printf("Child is executing...\n");

        exit(0);
    }

    printf("Parent process\n");
    printf("Child PID: %d\n", pid);

    waitpid(pid, NULL, 0);

    printf("Child execution completed\n");

    return 0;
}

/* ---------------- FORK + EXEC ---------------- */

static int builtin_forkexec(command_t *cmd)
{
    pid_t pid;

    if (cmd->argc > 1)
    {
        fprintf(stderr, "forkexec: no arguments required\n");
        return -1;
    }

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    /* Child */
    if (pid == 0)
    {
        printf("Child process created\n");
        printf("Child PID: %d\n", getpid());
        printf("Child executing: ls\n");

        execlp("ls", "ls", NULL);

        /* Only reached if exec fails */
        perror("exec");
        exit(1);
    }

    /* Parent */
    printf("Parent process\n");
    printf("Child PID: %d\n", pid);

    waitpid(pid, NULL, 0);

    printf("Child execution completed\n");

    return 0;
}

/* ---------------- CHECK BUILTIN ---------------- */

int is_builtin(const command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    if (strcmp(cmd->argv[0], "cd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "pwd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "echo") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "exit") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "fork") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "forkexec") == 0)
        return 1;

    return 0;
}

/* ---------------- EXECUTE BUILTIN ---------------- */

int execute_builtin(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return -1;

    if (strcmp(cmd->argv[0], "cd") == 0)
        return builtin_cd(cmd);

    if (strcmp(cmd->argv[0], "pwd") == 0)
        return builtin_pwd(cmd);

    if (strcmp(cmd->argv[0], "echo") == 0)
        return builtin_echo(cmd);

    if (strcmp(cmd->argv[0], "exit") == 0)
        return builtin_exit(cmd);

    if (strcmp(cmd->argv[0], "fork") == 0)
        return builtin_fork(cmd);

    if (strcmp(cmd->argv[0], "forkexec") == 0)
        return builtin_forkexec(cmd);

    return -1;
}
