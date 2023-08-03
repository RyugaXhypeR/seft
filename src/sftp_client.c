#include "sftp_client.h"

#include "debug.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/** SSH does not impose any restrictions on the passphrase length,
 * but for simplicity, we will have a finite length passphrase buffer. */
#define MAX_PASSPHRASE_LEN 128

ssh_session
do_ssh_init(char *host_name, uint8_t port_id) {
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
    result = ssh_userauth_publickey_auto(session, NULL, passphrase);
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
        DBG_ERR("Connection error: %s", ssh_get_error(session_ssh));
        sftp_free(session_sftp);
        clean_ssh_session(session_ssh);
        exit(EXIT_FAILURE);
    }

    return session_sftp;
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
