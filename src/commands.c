#include "commands.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** All the commands supported by the program. */
const char *COMMANDS[MAX_NUM_COMMANDS] = {"help",   "list", "create",
                                          "remove", "copy", "move"};
static CommandHandlerT *command_handlers[MAX_NUM_COMMANDS];
static size_t num_command_handler = 0;

CommandHandlerT *
CommandHandler_new(const char *command_name, const char *command_help_msg,
                   CommandStatusE (*command_handler)(ArgsT args)) {
    CommandHandlerT *self = malloc(sizeof *self);
    strncpy(self->command_name, command_name, MAX_COMMAND_NAME_LENGTH);
    strncpy(self->command_help_msg, command_help_msg, MAX_COMMAND_HELP_MSG_LENGTH);
    self->command_handler = command_handler;
    self->command_status = CMD_NOT_EXECUTED;
    return self;
}

void
CommandHandler_copy(CommandHandlerT *self, CommandHandlerT **dest) {
    *dest = CommandHandler_new(self->command_name, self->command_help_msg,
                               self->command_handler);
}

void
CommandHandler_free(CommandHandlerT *self) {
    free(self->command_name);
    free(self->command_help_msg);
    free(self);
}

void
CommandHandler_display_help_msg(CommandHandlerT *self) {
    printf("%s: %s\n", self->command_name, self->command_help_msg);
}

/** Helper function */
CommandHandlerT *
get_command_handler_from_name(const char *command_name) {
    for (size_t i = 0; i < num_command_handler; i++) {
        if (!strncmp(command_handlers[i]->command_name, command_name,
                     MAX_COMMAND_NAME_LENGTH)) {
            return command_handlers[i];
        }
    }
    return NULL;
}

void
__register_command_handler(CommandHandlerT *command_handler) {
    if (num_command_handler >= MAX_NUM_COMMANDS) {
        // TODO: handle this.
        return;
    }
    CommandHandler_copy(command_handler, &command_handlers[num_command_handler++]);
}

void
__register_commands(size_t num_args, ...) {
    va_list args;
    va_start(args, num_args);

    while (num_args--) {
        __register_command_handler(va_arg(args, CommandHandlerT *));
    }
    va_end(args);
}

/** Commands */
CommandStatusE
COMMAND(help)(ArgsT args) {
    CommandHandlerT *cmd;

    if (!args.num_args) {
        puts("Usage: sftp <cmd_name> [options]");
        for (size_t i = 0; i < num_command_handler; i++) {
            cmd = command_handlers[i];
            printf("\t");
            CommandHandler_display_help_msg(cmd);
        }
    } else {
        for (size_t i = 0; i < args.num_args; i++) {
            cmd = get_command_handler_from_name(args.args[i]);
            CommandHandler_display_help_msg(cmd);
        }
    }

    return CMD_OK;
}

CommandStatusE COMMAND(list)(ArgsT args);
CommandStatusE COMMAND(create)(ArgsT args);
CommandStatusE COMMAND(remove)(ArgsT args);
CommandStatusE COMMAND(copy)(ArgsT args);
CommandStatusE COMMAND(move)(ArgsT args);

void
run() {
    REGISTER(CommandHandler_new("help", "Prints this help message", COMMAND(help)),
             CommandHandler_new("list", "list ... TODO", COMMAND(list)),
             CommandHandler_new("delete", "delete ... TODO", COMMAND(remove)),
             CommandHandler_new("copy", "copy ... TODO", COMMAND(copy)),
             CommandHandler_new("move", "move ... TODO", COMMAND(move)));
}
