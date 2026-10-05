#include <stdio.h>

#include "lexer.h"
#include "token.h"

int lexer_validate(const token_list_t *list)
{
    int expecting_command = 1;
    int expecting_filename = 0;
    int command_found = 0;
    int background_found = 0;

    if (list == NULL || list->count == 0)
    {
        return 0;
    }

    for (int i = 0; i < list->count; i++)
    {
        const token_t *token = &list->tokens[i];

        switch (token->type)
        {
            case TOKEN_WORD:
                if (expecting_filename)
                {
                    expecting_filename = 0;
                }
                else
                {
                    expecting_command = 0;
                    command_found = 1;
                }
                break;

            case TOKEN_PIPE:
                if (!command_found || expecting_command ||
                    expecting_filename || background_found)
                {
                    fprintf(stderr,
                            "Lexer Error: Invalid pipe position\n");
                    return 0;
                }

                expecting_command = 1;
                command_found = 0;
                background_found = 0;
                break;

            case TOKEN_INPUT:
            case TOKEN_OUTPUT:
            case TOKEN_APPEND:
                if (expecting_filename)
                {
                    fprintf(stderr,
                            "Lexer Error: Missing filename before redirection\n");
                    return 0;
                }

                if (!command_found)
                {
                    fprintf(stderr,
                            "Lexer Error: Redirection before command\n");
                    return 0;
                }

                expecting_filename = 1;
                break;

            case TOKEN_BACKGROUND:
                if (!command_found || expecting_command ||
                    expecting_filename || background_found)
                {
                    fprintf(stderr,
                            "Lexer Error: Invalid background symbol\n");
                    return 0;
                }

                background_found = 1;
                break;

            case TOKEN_END:
                if (expecting_filename)
                {
                    fprintf(stderr,
                            "Lexer Error: Missing filename after redirection\n");
                    return 0;
                }

                if (!command_found || expecting_command)
                {
                    fprintf(stderr,
                            "Lexer Error: Missing command\n");
                    return 0;
                }

                return 1;

            case TOKEN_SYMBOL:
                fprintf(stderr,
                        "Lexer Error: Unsupported symbol '%s'\n",
                        token->text);
                return 0;

            case TOKEN_ERROR:
                fprintf(stderr,
                        "Lexer Error: Invalid token '%s'\n",
                        token->text);
                return 0;

            default:
                fprintf(stderr,
                        "Lexer Error: Unknown token\n");
                return 0;
        }
    }

    if (expecting_filename)
    {
        fprintf(stderr,
                "Lexer Error: Missing filename after redirection\n");
        return 0;
    }

    if (expecting_command || !command_found)
    {
        fprintf(stderr,
                "Lexer Error: Incomplete command\n");
        return 0;
    }

    return 1;
}
