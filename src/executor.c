
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "executor.h"
#include "builtin.h"

/* Redirect command input from a file */
static int apply_input_redirection(command_t *cmd)
{
    int fd;

    if (cmd->input[0] == '\0')
        return 0;

    fd = open(cmd->input, O_RDONLY);

    if (fd < 0)
    {
        perror(cmd->input);
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) < 0)
    {
        perror("dup2 input");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* Redirect command output to a file */
static int apply_output_redirection(command_t *cmd)
{
    int fd;
    int flags = O_WRONLY | O_CREAT;

    if (cmd->output[0] == '\0')
        return 0;

    if (cmd->append)
        flags |= O_APPEND;
    else
        flags |= O_TRUNC;

    fd = open(cmd->output, flags, 0644);

    if (fd < 0)
    {
        perror(cmd->output);
        return -1;
    }

    if (dup2(fd, STDOUT_FILENO) < 0)
    {
        perror("dup2 output");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* Restore standard input and output */
static void restore_stdio(int saved_stdin, int saved_stdout)
{
    if (saved_stdin >= 0)
    {
        if (dup2(saved_stdin, STDIN_FILENO) < 0)
            perror("restore stdin");
        close(saved_stdin);
    }

    if (saved_stdout >= 0)
    {
        if (dup2(saved_stdout, STDOUT_FILENO) < 0)
            perror("restore stdout");
        close(saved_stdout);
    }
}

/* Execute one command */
int execute_command(command_t *cmd)
{
    pid_t pid;
    int status;

    if (cmd == NULL || cmd->argc == 0)
        return -1;

    /* Run builtins in the parent to preserve shell state */
    if (is_builtin(cmd))
    {
        int saved_stdin;
        int saved_stdout;
        int result;

        fflush(NULL);

        saved_stdin = dup(STDIN_FILENO);
        saved_stdout = dup(STDOUT_FILENO);

        if (saved_stdin < 0 || saved_stdout < 0)
        {
            perror("dup");
            restore_stdio(saved_stdin, saved_stdout);
            return -1;
        }

        if (apply_input_redirection(cmd) < 0)
        {
            restore_stdio(saved_stdin, saved_stdout);
            return -1;
        }

        if (apply_output_redirection(cmd) < 0)
        {
            restore_stdio(saved_stdin, saved_stdout);
            return -1;
        }

        result = execute_builtin(cmd);

        fflush(stdout);
        fflush(stderr);

        restore_stdio(saved_stdin, saved_stdout);
        return result;
    }

    /* Flush buffered output before forking */
    fflush(NULL);

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        if (apply_input_redirection(cmd) < 0)
            _exit(1);

        if (apply_output_redirection(cmd) < 0)
            _exit(1);

        execvp(cmd->argv[0], cmd->argv);

        perror(cmd->argv[0]);
        _exit(127);
    }

    /* Background command: do not wait here */
    if (cmd->background)
    {
        printf("[Background process started: %d]\n", pid);
        return 0;
    }

    /* Wait for foreground command */
    if (waitpid(pid, &status, 0) < 0)
    {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status))
    {
        fprintf(stderr,
                "Process terminated by signal %d\n",
                WTERMSIG(status));
        return -1;
    }

    return 0;
}

/* Execute a pipeline of commands */
int execute_pipeline(pipeline_t *pipeline)
{
    int previous_read = -1;
    int command_count;
    int final_status = 0;
    pid_t pids[MAX_COMMANDS];

    if (pipeline == NULL)
        return -1;

    command_count = pipeline->command_count;

    if (command_count <= 0 || command_count > MAX_COMMANDS)
        return -1;

    /* A single command does not need a pipe */
    if (command_count == 1)
        return execute_command(&pipeline->commands[0]);

    fflush(NULL);

    for (int i = 0; i < command_count; i++)
    {
        int pipefd[2] = {-1, -1};

        /* Create a pipe except for the last command */
        if (i < command_count - 1)
        {
            if (pipe(pipefd) < 0)
            {
                perror("pipe");

                if (previous_read != -1)
                    close(previous_read);

                /* Wait for children already started */
                for (int j = 0; j < i; j++)
                    waitpid(pids[j], NULL, 0);

                return -1;
            }
        }

        pids[i] = fork();

        if (pids[i] < 0)
        {
            perror("fork");

            if (previous_read != -1)
                close(previous_read);

            if (i < command_count - 1)
            {
                close(pipefd[0]);
                close(pipefd[1]);
            }

            /* Wait for children already started */
            for (int j = 0; j < i; j++)
                waitpid(pids[j], NULL, 0);

            return -1;
        }

        if (pids[i] == 0)
        {
            command_t *cmd = &pipeline->commands[i];

            /* Connect previous command to standard input */
            if (previous_read != -1)
            {
                if (dup2(previous_read, STDIN_FILENO) < 0)
                {
                    perror("dup2 previous input");
                    _exit(1);
                }
            }

            /* Connect standard output to the next command */
            if (i < command_count - 1)
            {
                if (dup2(pipefd[1], STDOUT_FILENO) < 0)
                {
                    perror("dup2 pipe output");
                    _exit(1);
                }
            }

            /* Explicit redirections override pipeline defaults */
            if (apply_input_redirection(cmd) < 0)
                _exit(1);

            if (apply_output_redirection(cmd) < 0)
                _exit(1);

            /* Close unused pipe descriptors */
            if (previous_read != -1)
                close(previous_read);

            if (i < command_count - 1)
            {
                close(pipefd[0]);
                close(pipefd[1]);
            }

            /* Execute builtin or external command */
            if (is_builtin(cmd))
            {
                int result = execute_builtin(cmd);
                fflush(stdout);
                fflush(stderr);
                _exit(result == 0 ? 0 : 1);
            }

            execvp(cmd->argv[0], cmd->argv);

            perror(cmd->argv[0]);
            _exit(127);
        }

        /* Parent closes descriptors no longer needed */
        if (previous_read != -1)
            close(previous_read);

        if (i < command_count - 1)
        {
            close(pipefd[1]);
            previous_read = pipefd[0];
        }
        else
        {
            previous_read = -1;
        }
    }

    /* Background pipeline: return without waiting */
    if (pipeline->commands[command_count - 1].background)
    {
        printf("[Background pipeline started]\n");
        return 0;
    }

    /* Wait for every process in the foreground pipeline */
    for (int i = 0; i < command_count; i++)
    {
        int status;

        if (waitpid(pids[i], &status, 0) < 0)
        {
            perror("waitpid");
            final_status = -1;
            continue;
        }

        /* Return the status of the last command */
        if (i == command_count - 1)
        {
            if (WIFEXITED(status))
                final_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                final_status = -1;
        }
    }

    return final_status;
}
