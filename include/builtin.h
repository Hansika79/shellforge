#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"

/* Check whether command is a builtin */
int is_builtin(const command_t *cmd);

/* Execute builtin command */
int execute_builtin(command_t *cmd);

#endif
