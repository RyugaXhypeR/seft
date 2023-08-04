#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"


/** All the commands supported by the program. */
const char *COMMANDS[MAX_NUM_COMMANDS] = {"help",   "list", "create",
                                          "remove", "copy", "move"};
static CommandHandlerT COMMAND_HANDLERS[MAX_NUM_COMMANDS] = {
    {"help", "prints this message", COMMAND(help), CMD_NOT_EXECUTED},
    {"list", "list [OPTIONS]...", COMMAND(help), CMD_NOT_EXECUTED},
    {"create", "create [OPTIONS]...", COMMAND(help), CMD_NOT_EXECUTED},
    {"remove", "remove [OPTIONS]...", COMMAND(help), CMD_NOT_EXECUTED},
    {"copy", "copy [OPTIONS]...", COMMAND(help), CMD_NOT_EXECUTED},
    {"move", "move [OPTIONS]...", COMMAND(help), CMD_NOT_EXECUTED},
};
const int NUM_COMMAND_HANDLERS = (sizeof COMMAND_HANDLERS) / (sizeof *COMMAND_HANDLERS);

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
    free(self);
}

void
CommandHandler_display_help_msg(CommandHandlerT *self) {
    printf("%s: %s\n", self->command_name, self->command_help_msg);
}

/** Helper function */
CommandHandlerT *
get_command_handler_from_name(const char *command_name) {
    for (size_t i = 0; i < NUM_COMMAND_HANDLERS; i++) {
        if (!strncmp(COMMAND_HANDLERS[i].command_name, command_name,
                     MAX_COMMAND_NAME_LENGTH)) {
            return &COMMAND_HANDLERS[i];
        }
    }
    return NULL;
}

/** Commands */
CommandStatusE
COMMAND(help)(ArgsT args) {
    CommandHandlerT *cmd = malloc(sizeof *cmd);

    if (!args.num_args) {
        puts("Usage: sftp <cmd_name> [OPTIONS]");
        for (size_t i = 0; i < MAX_NUM_COMMANDS; i++) {
            *cmd = COMMAND_HANDLERS[i];
            printf("\t");
            CommandHandler_display_help_msg(cmd);
        }
    } else {
        for (size_t i = 0; i < args.num_args; i++) {
            cmd = get_command_handler_from_name(args.args[i]);
            if (cmd == NULL) {
                fprintf(stderr, "Invalid argument %s\n", args.args[i]);
                return CMD_INVALID_ARGS_TYPE;
            }

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
