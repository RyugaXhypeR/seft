#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "commands.h"
#include "debug.h"
#include "sftp_client.h"
#include "sftp_list.h"
#include "sftp_path.h"
#include "sftp_utils.h"

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
    uint8_t num_files_on_line = get_window_column_length() / 25;

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
copy_file_from_remote_to_local(ssh_session session_ssh, sftp_session session_sftp,
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
copy_dir_recursively(ssh_session session_ssh, sftp_session session_sftp,
                     char *abs_path_remote, char *abs_path_local) {
    ListT *sub_dir_path_stack = List_new(1, sizeof(char *));
    ListT *remote_dir;
    FileSystemT *file_system;
    char *dir_path_remote = NULL;
    char *dir_path_local = NULL;
    sftp_dir dir;

    do {
        if (dir_path_remote == NULL || dir_path_local == NULL) {
            dir_path_remote = abs_path_remote;
            dir_path_local = abs_path_local;
        } else {
            dir_path_remote = List_pop(sub_dir_path_stack);
            dir_path_local = path_replace_grand_parent(dir_path_remote, strlen(dir_path_remote), abs_path_local);
        }

        printf("mkdir: %s\n", dir_path_local);
        path_mkdir_parents(dir_path_local, strlen(dir_path_local));
        remote_dir = path_read_remote_dir(session_ssh, session_sftp, dir_path_remote);
        if (remote_dir == NULL) {
            return CMD_INTERNAL_ERROR;
        }

        for (size_t i = 0; i < remote_dir->length; i++) {
            file_system = List_get(remote_dir, i);

            switch (file_system->type) {
                case FS_REG_FILE:
                    copy_from_remote_to_local(
                        session_ssh, session_sftp, file_system->relative_path,
                        path_replace_grand_parent(file_system->relative_path,
                                                  strlen(file_system->relative_path),
                                                  abs_path_local));
                    break;
                case FS_DIRECTORY:
                    if (!path_is_dotted(file_system->name, strlen(file_system->name))) {
                        List_push(sub_dir_path_stack, file_system->relative_path);
                    }
                    break;
                case FS_SYM_LINK:
                    break; /* TODO */
                default:
                    DBG_ERR("Unknown type %d", file_system->type);
            }
        }

    } while (!List_is_empty(sub_dir_path_stack));

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
        return copy_dir_recursively(session_ssh, session_sftp, abs_path_remote,
                                    abs_path_local);
    } else if (from->type == SSH_FILEXFER_TYPE_REGULAR) {
        DBG_DEBUG("Copying file from %s to %s", abs_path_remote, abs_path_local);
        return copy_file_from_remote_to_local(session_ssh, session_sftp, abs_path_remote,
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
