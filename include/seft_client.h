#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include <stdint.h>

#include <libssh/sftp.h>
#include <libssh/libssh.h>

#include "seft_commands.h"


#define FLAG_LIST_BIT_POS_ALL 0x0

#define FLAG_LIST_BIT_POS_DIR_ONLY 0x1

#define FLAG_LIST_BIT_POS_FILE_ONLY 0x2

#define FLAG_LIST_BIT_POS_LONG_LIST 0x3

#define FLAG_LIST_BIT_POS_SORT 0x4

#define FLAG_LIST_BIT_POS_SORT_REVERSE 0x5

/** SSH FUNCTIONS */
ssh_session do_ssh_init(char *host_name, uint32_t port_id);
void clean_ssh_session(ssh_session session);
void clean_sftp_session(sftp_session session);

sftp_session do_sftp_init(ssh_session session_ssh);
CommandStatusE list_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                               char *directory, uint8_t flag);
CommandStatusE create_remote_file(ssh_session session_ssh, sftp_session session_sftp,
                                  char *abs_file_path);
CommandStatusE create_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                                 char *abs_dir_path);
CommandStatusE copy_from_remote_to_local(ssh_session session_ssh,
                                         sftp_session session_sftp, char *abs_path_remote,
                                         char *abs_path_local);
CommandStatusE copy_from_local_to_remote(ssh_session session_ssh,
                                         sftp_session session_sftp, char *abs_path_local,
                                         char *abs_path_remote);
#endif /* SFTP_CLIENT_H */
