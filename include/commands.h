#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>

/** A enum to represent the status of a command */
typedef enum {
    /** Command executed successfully */
    CMD_OK = 0,

    /** Invalid command name */
    CMD_INVALID_COMMAND,

    /** Invalid number of arguments */
    CMD_INVALID_ARGS_COUNT,

    /** Invalid argument type */
    CMD_INVALID_ARGS_TYPE,

    /** Internal error */
    CMD_INTERNAL_ERROR,

    /** Command not executed */
    CMD_NOT_EXECUTED = -1
} CommandStatusE;

#endif /* COMMANDS_H */
