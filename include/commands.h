#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>

#define MAX_COMMAND_NAME_LENGTH 64
#define MAX_COMMAND_HELP_MSG_LENGTH 1024
#define MAX_NUM_COMMANDS 6

#define COMMAND(name) cli_command_##name

typedef enum {
    CMD_OK = 0,
    CMD_INVALID_COMMAND,
    CMD_INVALID_ARGS_COUNT,
    CMD_INVALID_ARGS_TYPE,
    CMD_INTERNAL_ERROR,
    CMD_NOT_EXECUTED = -1
} CommandStatusE;

typedef struct {
    char **args;
    size_t num_args;
} ArgsT;

typedef struct {
    char command_name[MAX_COMMAND_NAME_LENGTH + 1];
    char command_help_msg[MAX_COMMAND_HELP_MSG_LENGTH + 1];
    CommandStatusE (*command_handler)(ArgsT args);
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
