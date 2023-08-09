#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "sftp_list.h"
#include "commands.h"
#include "debug.h"
#include "sftp_client.h"


/** SSH does not impose any restrictions on the passphrase length,
 * but for simplicity, we will have a finite length passphrase buffer. */
#define BUF_SIZE_PASSPHRASE 128

#define BUF_SIZE_FS_NAME 128
#define BUF_SIZE_FS_PATH 2048
#define BUF_SIZE_FILE_CONTENTS 65536

#define BIT_MATCH(bits, pos) (bits & (1 << pos))

/* File owner has perms to Read, Write and Execute the rest can only Read and Execute */
#define FS_CREATE_PERM S_IRWXU | S_IRWXG | S_IRWXO

#define __NUM_ARGS(type, ...) (sizeof((type []){__VA_ARGS__}) / sizeof(type))
#define FS_JOIN_PATH(...) __fs_path_join(__NUM_ARGS(char *, __VA_ARGS__), __VA_ARGS__)

char *
__fs_path_join(size_t num_paths, ...) {
    va_list args;
    size_t path_len = 0;
    char *path_buf = malloc(BUF_SIZE_FS_PATH * (sizeof *path_buf));
    char *path_name = malloc(BUF_SIZE_FS_NAME * (sizeof *path_buf));

    va_start(args, num_paths);
    for (size_t i = 0; i < num_paths; i++) {
        path_name = va_arg(args, char *);
        path_len += strlen(path_name);
        strncat(path_buf, path_name, path_len);
        strncat(path_buf, "/", 1);
    }
    va_end(args);

    return path_buf;
}

ssh_session
do_ssh_init(char *host_name, uint32_t port_id) {
    int8_t result;
    ssh_session session;
    char passphrase[BUF_SIZE_PASSPHRASE] = {0};

    ssh_init();

    session = ssh_new();
    if (session == NULL) {
        DBG_ERR("Couldn't create new ssh session: %s", ssh_get_error(session));
        exit(EXIT_FAILURE);
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host_name);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port_id);

    result = ssh_connect(session);
    if (result != SSH_OK) {
        DBG_ERR("Connection error: %s", ssh_get_error(session));
        clean_ssh_session(session);
        exit(EXIT_FAILURE);
    }

    ssh_getpass("Enter passphrase: ", passphrase, BUF_SIZE_PASSPHRASE, 0, 0);
    result = ssh_userauth_password(session, NULL, passphrase);
    if (result != SSH_AUTH_SUCCESS) {
        DBG_ERR("Authentication error: %s", ssh_get_error(session));
        clean_ssh_session(session);
        exit(EXIT_FAILURE);
    }

    return session;
}

sftp_session
do_sftp_init(ssh_session session_ssh) {
    int8_t result;
    sftp_session session_sftp;

    session_sftp = sftp_new(session_ssh);
    if (session_sftp == NULL) {
        DBG_ERR("Connection error: %s", ssh_get_error(session_ssh));
        clean_ssh_session(session_ssh);
        exit(EXIT_FAILURE);
    }

    result = sftp_init(session_sftp);
    if (result != SSH_OK) {
        DBG_ERR("Couldn't initialize SFTP session: Error Code %d",
                sftp_get_error(session_sftp));
        sftp_free(session_sftp);
        clean_ssh_session(session_ssh);
        exit(EXIT_FAILURE);
    }

    return session_sftp;
}

static uint32_t
_get_window_column_length(void) {
    struct winsize ws;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col;
}

static uint8_t
sftp_list_attr_check_show_hidden(sftp_attributes attr, uint8_t flag) {
    return BIT_MATCH(flag, 0) ? 1 : attr->name[0] != '.';
}

static uint8_t
sftp_list_attr_flag_check(sftp_attributes attr, uint8_t flag) {
    uint8_t is_valid = sftp_list_attr_check_show_hidden(attr, flag);

    if (BIT_MATCH(flag, 1)) {
        return attr->type == SSH_FILEXFER_TYPE_DIRECTORY && is_valid;
    }

    return is_valid;
}

CommandStatusE
list_remote_dir(ssh_session session_ssh, sftp_session session_sftp, char *directory,
                uint8_t flag) {
    int8_t result;
    sftp_dir dir;
    sftp_attributes attr;
    uint8_t num_files_on_line = _get_window_column_length() / 25;

    dir = sftp_opendir(session_sftp, directory);
    if (dir == NULL) {
        DBG_ERR("Couldn't open directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    if (!BIT_MATCH(flag, 2)) /* not list view */ {
        for (size_t i = 1; (attr = sftp_readdir(session_sftp, dir)) != NULL;) {
            if (sftp_list_attr_flag_check(attr, flag)) {
                printf("%-25s", attr->name);

                if (!(i++ % num_files_on_line)) {
                    puts("");
                }
            }
        }
    } else /* list view */ {
        while ((attr = sftp_readdir(session_sftp, dir)) != NULL) {
            if (sftp_list_attr_flag_check(attr, flag)) {
                printf("%-25s %-10s %zu\n", attr->name, attr->owner, attr->size);
            }
        }
    }

    if (!sftp_dir_eof(dir)) {
        DBG_ERR("Can't list directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }
    puts("");

    result = sftp_closedir(dir);
    if (result != SSH_FX_OK) {
        DBG_ERR("Can't close this directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    return CMD_OK;
}

CommandStatusE
create_remote_file(ssh_session session_ssh, sftp_session session_sftp,
                   char *abs_file_path) {
    sftp_file file =
        sftp_open(session_sftp, abs_file_path, O_CREAT | O_WRONLY, FS_CREATE_PERM);

    if (file == NULL) {
        DBG_ERR("Couldn't create file %s: %s", abs_file_path, ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    sftp_close(file);
    return CMD_OK;
}

CommandStatusE
create_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                  char *abs_dir_path) {
    int8_t result = sftp_mkdir(session_sftp, abs_dir_path, FS_CREATE_PERM);

    switch (result) {
        case SSH_FX_OK:
            return CMD_OK;
        case SSH_FX_FILE_ALREADY_EXISTS:
            DBG_INFO("Directory %s already exists", abs_dir_path);
            return CMD_OK;
        case SSH_FX_PERMISSION_DENIED:
            DBG_ERR("Permission Denied: directory %s could not be created", abs_dir_path);
            return CMD_INTERNAL_ERROR;
        default:
            DBG_ERR("Error: %s\n", ssh_get_error(session_ssh));
            return CMD_INTERNAL_ERROR;
    }
}

static CommandStatusE
_copy_file_from_remote_to_local(ssh_session session_ssh, sftp_session session_sftp,
                                char *abs_path_remote, char *abs_path_local) {
    int32_t num_bytes_read;
    char file_buf[BUF_SIZE_FILE_CONTENTS + 1];
    sftp_file from_file = sftp_open(session_sftp, abs_path_remote, O_RDONLY, 0);
    FILE *to_file = fopen(abs_path_local, "w");

    if (from_file == NULL) {
        DBG_ERR("Couldn't open file: %s", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    if (to_file == NULL) {
        DBG_ERR("Couldn't create file: %s", abs_path_local);
        return CMD_INTERNAL_ERROR;
    }

    while ((num_bytes_read = sftp_read(from_file, file_buf, BUF_SIZE_FILE_CONTENTS)) >
           0) {
        fwrite(file_buf, sizeof *file_buf, num_bytes_read, to_file);
    }

    if (num_bytes_read < 0) {
        DBG_ERR("Couldn't read remote file: Error Code: %d",
                sftp_get_error(session_sftp));
        return CMD_INTERNAL_ERROR;
    }

    return CMD_OK;
}

static CommandStatusE
_copy_dir_recursively(ssh_session session_ssh, sftp_session session_sftp,
                      char *abs_path_remote, char *abs_path_local) {
    sftp_attributes attr;
    sftp_dir from_dir = sftp_opendir(session_sftp, abs_path_remote);

    if (from_dir == NULL) {
        DBG_ERR("Couldn't open directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    while ((attr = sftp_readdir(session_sftp, from_dir)) != NULL) {
        switch (attr->type) {
            case SSH_FILEXFER_TYPE_REGULAR:
                _copy_file_from_remote_to_local(session_ssh, session_sftp, attr->name,
                                            abs_path_local);
                break;
            case SSH_FILEXFER_TYPE_DIRECTORY:
                /* Make this a non recursive function */
                _copy_dir_recursively(session_ssh, session_sftp, attr->name, abs_path_local);
                break;
            default:
                DBG_DEBUG("Ignoring type: %d", attr->type);
        }
    }

    sftp_closedir(from_dir);
    return CMD_OK;
}

CommandStatusE
copy_from_remote_to_local(ssh_session session_ssh, sftp_session session_sftp,
                          char *abs_path_remote, char *abs_path_local) {
    sftp_attributes from = sftp_stat(session_sftp, abs_path_remote);

    if (from == NULL) {
        DBG_ERR("Failed to get attributes: %s", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    if (from->type == SSH_FILEXFER_TYPE_DIRECTORY) {
        DBG_DEBUG("Copying dir from %s to %s", abs_path_remote, abs_path_local);
        return _copy_dir_recursively(session_ssh, session_sftp, abs_path_remote,
                                     abs_path_local);
    } else if (from->type == SSH_FILEXFER_TYPE_REGULAR) {
        DBG_DEBUG("Copying file from %s to %s", abs_path_remote, abs_path_local);
        return _copy_file_from_remote_to_local(session_ssh, session_sftp, abs_path_remote,
                                               abs_path_local);
    }

    return CMD_OK;
}

void
clean_ssh_session(ssh_session session) {
    DBG_DEBUG("Freeing session: %p", (void *)session);

    if (ssh_is_connected(session)) {
        ssh_disconnect(session);
    }
    ssh_free(session);
    ssh_finalize();
}

void
clean_sftp_session(sftp_session session) {
    DBG_DEBUG("Freeing session: %p", (void *)session);

    sftp_free(session);
}
