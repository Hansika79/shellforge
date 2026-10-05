#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "token.h"

void token_list_init(token_list_t *list)
{
    list->count = 0;
}

void token_add(token_list_t *list,
               token_type_t type,
               const char *text)
{
    if (list->count >= MAX_TOKENS)
    {
        return;
    }

    token_t *t = &list->tokens[list->count];

    t->type = type;

    strncpy(t->text, text, MAX_TOKEN_LEN - 1);
    t->text[MAX_TOKEN_LEN - 1] = '\0';

    list->count++;
}

static const char *token_name(token_type_t type)
{
    switch (type)
    {
        case TOKEN_WORD:
            return "WORD";

        case TOKEN_PIPE:
            return "PIPE";

        case TOKEN_INPUT:
            return "INPUT";

        case TOKEN_OUTPUT:
            return "OUTPUT";

        case TOKEN_APPEND:
            return "APPEND";

        case TOKEN_BACKGROUND:
            return "BACKGROUND";

        case TOKEN_SYMBOL:
            return "SYMBOL";

        case TOKEN_END:
            return "END";

        case TOKEN_ERROR:
            return "ERROR";

        default:
            return "UNKNOWN";
    }
}

void tokenize(const char *line, token_list_t *list)
{
    char word[MAX_TOKEN_LEN];
    int pos = 0;

    for (int i = 0; ; i++)
    {
        char c = line[i];

        /* End of input */
        if (c == '\0')
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
            }

            token_add(list, TOKEN_END, "END");
            break;
        }

        /* Space */
        if (isspace((unsigned char)c))
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
                pos = 0;
            }

            continue;
        }

        /* Pipe | */
        if (c == '|')
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
                pos = 0;
            }

            token_add(list, TOKEN_PIPE, "|");
            continue;
        }

        /* Input < */
        if (c == '<')
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
                pos = 0;
            }

            token_add(list, TOKEN_INPUT, "<");
            continue;
        }

        /* Output > and Append >> */
        if (c == '>')
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
                pos = 0;
            }

            if (line[i + 1] == '>')
            {
                token_add(list, TOKEN_APPEND, ">>");
                i++;
            }
            else
            {
                token_add(list, TOKEN_OUTPUT, ">");
            }

            continue;
        }

        /* Background & */
        if (c == '&')
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
                pos = 0;
            }

            token_add(list, TOKEN_BACKGROUND, "&");
            continue;
        }

        /* Symbols @ # $ % ^ * ( ) */
        if (c == '@' ||
            c == '#' ||
            c == '%' ||
            c == '^' ||
            c == '*' ||
            c == '(' ||
            c == ')')
        {
            if (pos > 0)
            {
                word[pos] = '\0';
                token_add(list, TOKEN_WORD, word);
                pos = 0;
            }

            char symbol[2];

            symbol[0] = c;
            symbol[1] = '\0';

            token_add(list, TOKEN_SYMBOL, symbol);

            continue;
        }

        /* Normal character */
        if (pos < MAX_TOKEN_LEN - 1)
        {
            word[pos++] = c;
        }
        else
        {
            token_add(list, TOKEN_ERROR, "WORD_TOO_LONG");
            return;
        }
    }
}

void token_print(const token_list_t *list)
{
    printf("\n---------- TOKENS ----------\n");

    for (int i = 0; i < list->count; i++)
    {
        printf("%2d : %-12s %s\n",
               i,
               token_name(list->tokens[i].type),
               list->tokens[i].text);
    }

    printf("----------------------------\n");
}
