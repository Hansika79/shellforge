
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "token.h"
#include "history.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"

/* Collect completed child processes without blocking */
static void reap_background_processes(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("[Background process %d completed]\n", pid);
    }
}

int main(void)
{
    char *line;

    while (1)
    {
        line = readline("shellforge$ ");

        /* Reap finished background children after input returns */
        reap_background_processes();

        if (line == NULL)
        {
            printf("\n");
            break;
        }

        if (line[0] == '\0')
        {
            free(line);
            continue;
        }

        if (strcmp(line, "exit") == 0)
        {
            free(line);
            break;
        }

        if (strcmp(line, "history") == 0)
        {
            print_history();
            free(line);
            continue;
        }

        add_history(line);

        token_list_t list;
        token_list_init(&list);

        tokenize(line, &list);

        pipeline_t pipeline;

        if (lexer_validate(&list))
        {
            if (parser(&list, &pipeline))
            {
                expand_variables(&pipeline);
                execute_pipeline(&pipeline);
            }
        }

        /* Also collect children that finished during command execution */
        reap_background_processes();

        free(line);
    }

    /* One final non-blocking cleanup */
    reap_background_processes();

    return 0;
}
