#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include <stdint.h>

#include <libssh/sftp.h>
#include <libssh/libssh.h>

#include "commands.h"

/** SSH FUNCTIONS */
ssh_session do_ssh_init(char *host_name, uint32_t port_id);
void clean_ssh_session(ssh_session session);
void clean_sftp_session(sftp_session session);

sftp_session do_sftp_init(ssh_session session_ssh);
CommandStatusE list_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                               char *directory, uint8_t flag);
CommandStatusE create_remote_file(ssh_session session_ssh, sftp_session session_sftp,
                                  char *current_working_directory, char *filename);
CommandStatusE create_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                                 char *current_working_directory, char *dirname);
#endif /* SFTP_CLIENT_H */
