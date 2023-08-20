#include <argp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libssh/sftp.h>

#include "commands.h"
#include "config.h"
#include "debug.h"
#include "sftp_ansi_colors.h"
#include "sftp_client.h"
#include "sftp_utils.h"

#define MAX_NUM_COMMANDS 128

const char *argp_program_version = "SFTP-CLI 0.1";

static char doc_header_connect[] =
    "Interact with SFTP servers via command-line interface";
static char doc_connect[] = "[-s] <subsystem/sftp-server> -p <port>";
static struct argp_option option_connect[] = {
    {"subsystem", 's', "SUBSYSTEM", 0, "Specify the server subsystem to connect to", 0},
    {"port", 'p', "PORT", 0, "Port number of the server", 0},
    {0},
};

static char doc_header_list[] = "List files and directories";
static char doc_list[] = "[<dir>] [OPTIONS]";
static struct argp_option option_list[] = {
    {"long", 'l', "LONG_LISTING", OPTION_ARG_OPTIONAL, "List in long listing format", 0},
    {"dir", 'd', "DIR_ONLY", OPTION_ARG_OPTIONAL, "List directories only", 0},
    {"file", 'f', "FILE_ONLY", OPTION_ARG_OPTIONAL, "List files only", 0},
    {"all", 'a', "SHOW_ALL", OPTION_ARG_OPTIONAL, "Show hidden and non-hidden files", 0},
    {"reverse", 'r', "REVERSE", OPTION_ARG_OPTIONAL, "Display in reverse order", 0},
    {"sort", 's', "SORT", OPTION_ARG_OPTIONAL, "Sort by specified field", 0},
    {"help", 'h', "HELP", OPTION_ARG_OPTIONAL, "Show help documentation", 0},
    {0},
};

static char doc_header_copy[] =
    "Synchronize filesystem bidirectionally between remote and local destinations.";
static char doc_copy[] = "[OPTIONS]";
static struct argp_option option_copy[] = {
    {"local", 'l', 0, 0, "Copy filesystem object to the local computer", 0},
    {"remote", 'r', 0, 0, "Copy filesystem object to the remote server", 0},
    {0},
};

static char doc_header_create[] = "Create files and directories on remote server";
static char doc_create[] = "[OPTIONS]";
static struct argp_option option_create[] = {
    {"dir", 'd', 0, 0, "Create a directory", 0},
    {"file", 'f', 0, 0, "Create a file", 0},
    {"local", 'l', 0, 0, "Create filesystem object on the local computer", 0},
    {"remote", 'r', 0, 0, "Create filesystem object on the remote server", 0},
    {0},
};

typedef struct {
    char *host;
    uint32_t port;
} ConnectArgsT;

typedef struct {
    char *dir;
    uint8_t flag;
} ListArgsT;

typedef struct {
#define FLAG_COPY_BIT_POS_IS_SET 0x0
#define FLAG_COPY_BIT_POS_IS_REMOTE 0x1
    uint8_t flag;
    char *source;
    char *dest;
} CopyArgsT;

typedef struct {
#define FLAG_CREATE_BIT_POS_IS_SET 0x0
#define FLAG_CREATE_BIT_POS_IS_REMOTE 0x1
#define FLAG_CREATE_BIT_POS_IS_DIR 0x2
    uint8_t flag;
    char *filesystem;
} CreateArgsT;

static ssh_session session_ssh = NULL;
static sftp_session session_sftp = NULL;

char **
get_arg_vec(char *input, int32_t *length) {
    static char *arg_vec[MAX_NUM_COMMANDS + 1];
    *length = 0;

    for (char *token = strtok(input, " "); token != NULL; token = strtok(NULL, " ")) {
        arg_vec[(*length)++] = strdup(token);
    }

    return arg_vec;
}

static error_t
parse_option_list(int32_t key, char *arg, struct argp_state *state) {
    ListArgsT *args = state->input;

    switch (key) {
        case 'l':
            BIT_SET(args->flag, FLAG_LIST_BIT_POS_LONG_LIST);
            break;
        case 'd':
            BIT_SET(args->flag, FLAG_LIST_BIT_POS_DIR_ONLY);
            break;
        case 'a':
            BIT_SET(args->flag, FLAG_LIST_BIT_POS_ALL);
            break;
        case 'f':
            BIT_SET(args->flag, FLAG_LIST_BIT_POS_FILE_ONLY);
            break;
        case 'r':
            BIT_SET(args->flag, FLAG_LIST_BIT_POS_SORT_REVERSE);
            break;
        case 's':
            BIT_SET(args->flag, FLAG_LIST_BIT_POS_SORT);
            break;
        case 'h':
            argp_state_help(state, stdout,
                            ARGP_HELP_DOC | ARGP_HELP_USAGE | ARGP_HELP_LONG);
            break;
        case ARGP_KEY_END:
            if (state->argc < 2) {
                argp_state_help(state, stdout,
                                ARGP_HELP_DOC | ARGP_HELP_USAGE | ARGP_HELP_LONG);
            }
            break;
        case ARGP_KEY_ARG:
            args->dir = strdup(arg);
            break;
    }

    return 0;
}

static error_t
parse_option_copy(int32_t key, char *arg, struct argp_state *state) {
    CopyArgsT *args = state->input;

    switch (key) {
        case 'r':
            if (!BIT_MATCH(args->flag, FLAG_CREATE_BIT_POS_IS_SET)) {
                args->flag = (1 << FLAG_CREATE_BIT_POS_IS_REMOTE) | 1;
            }
            break;
        case 'l':
            if (!BIT_MATCH(args->flag, FLAG_CREATE_BIT_POS_IS_SET)) {
                args->flag = (1 << !FLAG_CREATE_BIT_POS_IS_REMOTE) | 1;
            }
            break;
        case 'd':
            BIT_SET(args->flag, FLAG_CREATE_BIT_POS_IS_DIR);
            break;
        case 'f':
            BIT_CLEAR(args->flag, FLAG_CREATE_BIT_POS_IS_DIR);
            break;
        case 'h':
            argp_state_help(state, stdout,
                            ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_USAGE);
            break;
        case ARGP_KEY_END:
            if (state->argc < 2) {
                argp_state_help(state, stdout,
                                ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_USAGE);
            }
            break;
        case ARGP_KEY_ARG: {
            if (arg == NULL) {
                break;
            }
            if (state->arg_num == 0) {
                args->source = strdup(arg);
            } else if (state->arg_num == 1) {
                args->dest = strdup(arg);
            }
            break;
        }
    }

    return 0;
}

static error_t
parse_option_create(int32_t key, char *arg, struct argp_state *state) {
    CreateArgsT *args = state->input;

    switch (key) {
        case 'r':
            if (!BIT_MATCH(args->flag, FLAG_COPY_BIT_POS_IS_SET)) {
                args->flag = (1 << FLAG_COPY_BIT_POS_IS_REMOTE) | 1;
            }
            break;
        case 'l':
            if (!BIT_MATCH(args->flag, FLAG_COPY_BIT_POS_IS_SET)) {
                args->flag = (1 << !FLAG_COPY_BIT_POS_IS_REMOTE) | 1;
            }
            break;
        case 'h':
            argp_state_help(state, stdout,
                            ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_USAGE);
            break;
        case ARGP_KEY_END:
            if (state->argc < 2) {
                argp_state_help(state, stdout,
                                ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_USAGE);
            }
            break;
        case ARGP_KEY_ARG: {
            if (arg != NULL) {
                args->filesystem = strdup(arg);
            }
            break;
        }
    }
    return 0;
}

static error_t
parse_option_connect(int32_t key, char *arg, struct argp_state *state) {
    ConnectArgsT *args = state->input;

    switch (key) {
        case 's':
            args->host = strdup(arg);
            break;
        case 'p':
            args->port = atoi(arg);
            break;
        case 'h':
            argp_state_help(state, stdout,
                            ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_USAGE);
            break;
        case ARGP_KEY_END:
            if (state->argc < 2) {
                argp_state_help(state, stdout,
                                ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_USAGE);
            }
            break;
    }

    return 0;
}

static CommandStatusE
subcommand_dispatcher(char **arg_vec, uint32_t length) {
    char *subcommand;
    struct argp arg_parser;

    if (length < 1) {
        return CMD_INVALID_ARGS_COUNT;
    }

    subcommand = arg_vec[0];
    if (!strcmp(subcommand, "list")) {
        ListArgsT list_args = {NULL, 0};

        arg_parser = (struct argp){
            option_list, parse_option_list, doc_list, doc_header_list, 0, 0, 0};
        argp_parse(&arg_parser, length, arg_vec, 0, 0, &list_args);

        /* Print help message and continue */
        if (length == 1) {
            return CMD_OK;
        }

        if (list_args.dir == NULL) {
            return CMD_INVALID_ARGS_TYPE;
        }
        list_remote_dir(session_ssh, session_sftp, list_args.dir, list_args.flag);

        free(list_args.dir);

    } else if (!strcmp(subcommand, "copy")) {
        CopyArgsT copy_args = {0, NULL, NULL};

        arg_parser = (struct argp){
            option_copy, parse_option_copy, doc_copy, doc_header_copy, 0, 0, 0};
        argp_parse(&arg_parser, length, arg_vec, 0, 0, &copy_args);

        /* Print help message and continue */
        if (length == 1) {
            return CMD_OK;
        }

        if (copy_args.source == NULL || copy_args.dest == NULL) {
            return CMD_INVALID_ARGS_TYPE;
        }

        if (BIT_MATCH(copy_args.flag, FLAG_COPY_BIT_POS_IS_REMOTE)) {
            copy_from_remote_to_local(session_ssh, session_sftp, copy_args.source,
                                      copy_args.dest);
        } else {
            copy_from_local_to_remote(session_ssh, session_sftp, copy_args.source,
                                      copy_args.dest);
        }

        free(copy_args.source);
        free(copy_args.dest);

    } else if (!strcmp(subcommand, "create")) {
        CreateArgsT create_args = {0, NULL};

        arg_parser = (struct argp){
            option_create, parse_option_create, doc_create, doc_header_create, 0, 0, 0};
        argp_parse(&arg_parser, length, arg_vec, 0, 0, &create_args);

        /* Print help message and continue */
        if (length == 1) {
            return CMD_OK;
        }

        if (create_args.filesystem == NULL) {
            return CMD_INVALID_ARGS_TYPE;
        }

        if (BIT_MATCH(create_args.flag, FLAG_CREATE_BIT_POS_IS_REMOTE)) {
            if (BIT_MATCH(create_args.flag, FLAG_CREATE_BIT_POS_IS_DIR)) {
                create_remote_dir(session_ssh, session_sftp, create_args.filesystem);
            } else {
                create_remote_file(session_ssh, session_sftp, create_args.filesystem);
            }
        } else { /* TODO */
        }

        free(create_args.filesystem);

    } else if (!strcmp(subcommand, "connect")) {
        ConnectArgsT connect_args = {NULL, 0};

        arg_parser = (struct argp){option_connect,
                                   parse_option_connect,
                                   doc_connect,
                                   doc_header_connect,
                                   0,
                                   0,
                                   0};
        argp_parse(&arg_parser, length, arg_vec, 0, 0, &connect_args);

        /* Print help message and continue */
        if (length == 1) {
            return CMD_OK;
        }

        if (connect_args.host == NULL) {
            return CMD_INVALID_ARGS_TYPE;
        }

        session_ssh = do_ssh_init(connect_args.host, connect_args.port);
        session_sftp = do_sftp_init(session_ssh);

        free(connect_args.host);
    } else {
        return CMD_INVALID_COMMAND;
    }
    return CMD_OK;
}

int
main(int length, char *arg_vec[]) {
    char input[4096];
    CommandStatusE result;

    /* Skipping file name */
    arg_vec++;
    length--;

    for (;;) {
        result = subcommand_dispatcher(arg_vec, length);

        if (result == CMD_INVALID_ARGS_COUNT) {
            DBG_ERR("Invalid number of arguments provided: %d", length);
        } else if (result == CMD_INVALID_ARGS_TYPE) {
            DBG_ERR("Invalid type of arguments provided %s", "");
        } else if (result == CMD_INVALID_COMMAND) {
            DBG_ERR("Invalid command name: " ANSI_FG_GREEN "%s" ANSI_RESET, arg_vec[0]);
        }

        printf(REPL_PROMPT);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        length = strlen(input);
        arg_vec = get_arg_vec(input, &length);
    }

    if (session_sftp != NULL && session_ssh != NULL) {
        DBG_INFO("Cleaning up ssh and sftp sessions: %s", "");
        clean_sftp_session(session_sftp);
        clean_ssh_session(session_ssh);
    }

    return 0;
}
