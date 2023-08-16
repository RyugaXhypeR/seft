#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>

#define MAX_COMMAND_NAME_LENGTH 64
#define MAX_COMMAND_HELP_MSG_LENGTH 1024
#define MAX_NUM_COMMANDS 6

#define COMMAND(name) cli_command_##name

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

/** A structure to hold the arguments passed to a command */
typedef struct {
    /** Arguments passed to subcommands of the program. */
    char **args;

    /* Number of arguments passed to subcommands of the program. */
    size_t num_args;
} ArgsT;

/** A structure to hold information about a command */
typedef struct {
    /** Name of the command */
    char command_name[MAX_COMMAND_NAME_LENGTH + 1];

    /** Help message for the command */
    char command_help_msg[MAX_COMMAND_HELP_MSG_LENGTH + 1];

    /** Function pointer to the command handler */
    CommandStatusE (*command_handler)(ArgsT args);

    /** Status of the command */
    CommandStatusE command_status;
} CommandHandlerT;

CommandHandlerT *CommandHandler_new(const char *command_name,
                                    const char *command_help_msg,
                                    CommandStatusE (*command_handler)(ArgsT args));
void CommandHandler_free(CommandHandlerT *self);
void CommandHandler_display_help_msg(CommandHandlerT *self);

/** Helper function */
CommandHandlerT *get_command_handler_from_name(const char *command_name);

/** Commands */
CommandStatusE COMMAND(help)(ArgsT args);
CommandStatusE COMMAND(list)(ArgsT args);
CommandStatusE COMMAND(create)(ArgsT args);
CommandStatusE COMMAND(remove)(ArgsT args);
CommandStatusE COMMAND(copy)(ArgsT args);
CommandStatusE COMMAND(move)(ArgsT args);

#endif /* COMMANDS_H */
