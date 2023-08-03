#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <stdbool.h>
#include <stdint.h>

/** SSH FUNCTIONS */
ssh_session do_ssh_init(char *host_name, uint8_t port_id);
void clean_ssh_session(ssh_session session);

sftp_session do_sftp_init(ssh_session session_ssh);

#endif /* !SFTP_CLIENT_H */
