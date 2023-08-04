#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include <stdint.h>

#include <libssh/sftp.h>
#include <libssh/libssh.h>


/** SSH FUNCTIONS */
ssh_session do_ssh_init(char *host_name, uint32_t port_id);
void clean_ssh_session(ssh_session session);

sftp_session do_sftp_init(ssh_session session_ssh);

#endif  /* SFTP_CLIENT_H */
