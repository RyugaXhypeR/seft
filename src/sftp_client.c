#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "commands.h"
#include "debug.h"
#include "sftp_client.h"


/** SSH does not impose any restrictions on the passphrase length,
 * but for simplicity, we will have a finite length passphrase buffer. */
#define BUF_SIZE_PASSPHRASE 128

#define BUF_SIZE_FS_PATH 1024

#define BIT_MATCH(bits, pos) (bits & (1 << pos))

/* File owner has perms to Read, Write and Execute the rest can only Read and Execute */
#define FS_CREATE_PERM S_IRWXU | S_IRWXG | S_IRWXO

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
        return S_ISDIR(attr->type) && is_valid;
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
    if (result != SSH_OK) {
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

    if (result != SSH_OK) {
        DBG_ERR("Couldn't create directory: %s", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
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
