#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "commands.h"
#include "debug.h"
#include "sftp_client.h"

/** SSH does not impose any restrictions on the passphrase length,
 * but for simplicity, we will have a finite length passphrase buffer. */
#define MAX_PASSPHRASE_LEN 128

#define BIT_MATCH(bits, pos) (bits & (1 << pos))

ssh_session
do_ssh_init(char *host_name, uint32_t port_id) {
    uint8_t result;
    ssh_session session;
    char passphrase[MAX_PASSPHRASE_LEN] = {0};

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

    ssh_getpass("Enter passphrase: ", passphrase, MAX_PASSPHRASE_LEN, 0, 0);
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
    uint8_t result;
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
__get_window_coulmn_length() {
    struct winsize ws;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col;
}

uint8_t
sftp_list_attr_check_show_hidden(sftp_attributes attr, uint8_t flag) {
    return BIT_MATCH(flag, 0) ? 1 : attr->name[0] != '.';
}

uint8_t
sftp_list_attr_flag_check(sftp_attributes attr, uint8_t flag) {
    uint8_t is_valid = sftp_list_attr_check_show_hidden(attr, flag);

    if (BIT_MATCH(flag, 1) && attr->type == 2 && is_valid) {
        is_valid = 1;
    }

    return is_valid;
}

CommandStatusE
print_sftp_list(ssh_session session_ssh, sftp_session session_sftp, char *directory,
                uint8_t flag) {
    uint8_t result, to_print;
    sftp_dir dir;
    sftp_attributes attr;

    dir = sftp_opendir(session_sftp, directory);
    if (dir == NULL) {
        DBG_ERR("Couldn't open directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    if (!BIT_MATCH(flag, 2)) /* not list view */ {
        for (size_t i = 0; (attr = sftp_readdir(session_sftp, dir)) != NULL;) {
            to_print = sftp_list_attr_flag_check(attr, flag);

            if (!(i % 3)) puts("");

            if (to_print) {
                printf("%-25s", attr->name);
                i++;
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

    result = sftp_closedir(dir);
    if (result != SSH_OK) {
        DBG_ERR("Can't close this directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    return CMD_OK;
}

void
clean_ssh_session(ssh_session session) {
    DBG_DEBUG("Freeing session: %p", session);
    if (ssh_is_connected(session)) {
        ssh_disconnect(session);
    }
    ssh_free(session);
    ssh_finalize();
}
