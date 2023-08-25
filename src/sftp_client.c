#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "commands.h"
#include "debug.h"
#include "sftp_ansi_colors.h"
#include "sftp_client.h"
#include "sftp_list.h"
#include "sftp_path.h"
#include "sftp_utils.h"
#include "config.h"

/**
 * Function to initialize ssh session.
 *
 * :param host_name: Host name to connect to.
 * :param port_id: Port number to connect to.
 *
 * :return: ssh_session object.
 *
 * .. note:: This function will exit the program if any error occurs.
 *
 * .. warning:: This function will ask for passphrase from the user.
 *    If the user enters wrong passphrase, the program will exit.
 */
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

    printf((ANSI_FG_GREEN "%s's passphrase: " ANSI_RESET), host_name);
    ssh_getpass("", passphrase, BUF_SIZE_PASSPHRASE, 0, 0);
    result = ssh_userauth_password(session, NULL, passphrase);
    if (result != SSH_AUTH_SUCCESS) {
        DBG_ERR("Authentication error: %s", ssh_get_error(session));
        clean_ssh_session(session);
        exit(EXIT_FAILURE);
    }

    return session;
}

/**
 * Function to clean ssh session.
 *
 * :param session: ssh_session object.
 *
 * .. note:: This function will exit the program if any error occurs.
 */
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

/**
 * Helper function to print files/directories in list view.
 *
 * :param session_ssh: ssh_session object.
 * :param session_sftp: sftp_session object.
 * :param directory: Directory to list.
 * :param flag: A bit mask to specify the type of listing.
 *     If 0th bit is set, then list all files/directories in the directory.
 *     If 1st bit is set, then list subdirectories.
 *     If 2nd bit is set, then list files/directories in list view.
 * */
CommandStatusE
list_remote_dir(ssh_session session_ssh, sftp_session session_sftp, char *directory,
                uint8_t flag) {
    sftp_dir dir;
    sftp_attributes attr;
    FileSystemT *fs;
    ListT *dir_contents;
    ListT *formatted_contents = List_new(1, sizeof(char *));
    char *filename = dbg_calloc(BUF_SIZE_FS_NAME, sizeof *filename);
    size_t width_screen = get_window_column_length();

    dir = sftp_opendir(session_sftp, directory);
    if (dir == NULL) {
        DBG_ERR("Couldn't open directory: %s\n", ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    if (BIT_MATCH(flag, FLAG_LIST_BIT_POS_LONG_LIST)) /* list view */ {
        while ((attr = sftp_readdir(session_sftp, dir)) != NULL) {
            if (check_path_type(attr->name, strlen(attr->name),
                                attr->type == SSH_FILEXFER_TYPE_DIRECTORY, flag)) {
                printf("%-25s %-10s %zu\n", attr->name, attr->owner, attr->size);
            }
        }
        return CMD_OK;
    }

    sftp_closedir(dir);

    dir_contents = path_read_remote_dir(session_ssh, session_sftp, directory);
    for (size_t i = 0; i < dir_contents->length; i++) {
        fs = List_get(dir_contents, i);
        if (fs->type == FS_DIRECTORY) {
            sprintf(filename, (COLOR_FOLDER ICON_FOLDER " %s" ANSI_RESET), fs->name);
        } else {
            sprintf(filename, (COLOR_FILE ICON_FILE " %s" ANSI_RESET), fs->name);
        }
        if (check_path_type(fs->name, strlen(fs->name), fs->type == FS_DIRECTORY, flag)) {
            List_push(formatted_contents, filename, strlen(filename) + 1);
        }
    }

    if (List_length(formatted_contents)) {
        char_list_format_columnwise(formatted_contents, width_screen, "    ");
    }

    FileSystem_list_free(dir_contents);
    List_free(formatted_contents);
    dbg_safe_free(filename);
    return CMD_OK;
}

/**
 * Helper function to create a file on the remote server.
 *
 * :param session_ssh: ssh_session object.
 * :param session_sftp: sftp_session object.
 * :param abs_file_path: Absolute path of the file to be created.
 */
CommandStatusE
create_remote_file(ssh_session session_ssh, sftp_session session_sftp,
                   char *abs_file_path) {
    sftp_file file =
        sftp_open(session_sftp, abs_file_path, O_CREAT | O_WRONLY, FS_CREATE_PERM);

    if (file == NULL) {
        DBG_ERR("Couldn't create file %s: %s", abs_file_path, ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    DBG_INFO("Created file: %s", abs_file_path);
    sftp_close(file);
    return CMD_OK;
}

/**
 * Helper function to create a directory on the remote server.
 *
 * :param session_ssh: ssh_session object.
 * :param session_sftp: sftp_session object.
 * :param abs_dir_path: Absolute path of the directory to be created.
 */
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
create_parents_remote(ssh_session session_ssh, sftp_session session_sftp,
                      char *path_str) {
    char *path_buf = dbg_malloc(BUF_SIZE_FS_PATH);
    ListT *path_list = path_split(path_str, strlen(path_str));
    int8_t result;

    path_buf = List_get(path_list, 0);
    for (size_t i = 0; i < path_list->length; i++) {
        if (i) {
            FS_JOIN_PATH(path_buf, List_get(path_list, i));
        }

        result = sftp_mkdir(session_sftp, path_buf, FS_CREATE_PERM);
        if (!result) {
            continue;
        }


        switch (sftp_get_error(session_sftp)) {
            case SSH_FX_FILE_ALREADY_EXISTS:
                DBG_INFO("Directory %s already exists", path_buf);
                break;
            case SSH_FX_PERMISSION_DENIED:
                DBG_ERR("Permission Denied: directory %s could not be created", path_buf);
                break;
            default:
                DBG_ERR("Error while creating parent:%s:: %s\n", path_buf,
                        ssh_get_error(session_ssh));
                break;
        }
    }

    dbg_safe_free(path_buf);
    List_free(path_list);

    return CMD_OK;
}

/**
 * Helper function to copy a file from local to remote server.
 *
 * :param session_ssh: ssh_session object.
 * :param session_sftp: sftp_session object.
 * :param abs_path_local: Absolute path of the file on local machine.
 * :param abs_path_remote: Absolute path of the file on remote machine.
 */
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

    sftp_close(from_file);
    fclose(to_file);

    return CMD_OK;
}

static CommandStatusE
copy_file_from_local_to_remote(ssh_session session_ssh, sftp_session session_sftp,
                               char *abs_path_local, char *abs_path_remote) {
    int32_t num_bytes_read;
    char file_buf[BUF_SIZE_FILE_CONTENTS + 1] = {0};
    struct stat from_file_stat;
    FILE *from_file;
    sftp_file to_file;

    stat(abs_path_local, &from_file_stat);

    /* Not really sure why this is needed but, it doesn't work without it
     * so  ¯\_(ツ)_/¯ */
    if (!from_file_stat.st_size) {
        DBG_INFO("File with 0 size: %s", abs_path_local);
        create_remote_file(session_ssh, session_sftp, abs_path_remote);
        return CMD_OK;
    }

    from_file = fopen(abs_path_local, "r");
    to_file =
        sftp_open(session_sftp, abs_path_remote, O_CREAT | O_WRONLY, FS_CREATE_PERM);

    if (from_file == NULL) {
        DBG_ERR("Couldn't open file: %s", abs_path_local);
        return CMD_INTERNAL_ERROR;
    }

    if (to_file == NULL) {
        DBG_ERR("Couldn't create file: %s: %s", abs_path_remote,
                ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    while ((num_bytes_read = fread(file_buf, sizeof *file_buf, BUF_SIZE_FILE_CONTENTS,
                                   from_file)) > 0) {
        sftp_write(to_file, file_buf, num_bytes_read);
    }

    if (num_bytes_read < 0) {
        DBG_ERR("Couldn't read local file: Error Code: %d", errno);
        return CMD_INTERNAL_ERROR;
    }

    fclose(from_file);
    sftp_close(to_file);

    return CMD_OK;
}

/**
 * Helper function to copy a file from remote to local server.
 *
 * :param session_ssh: ssh_session object.
 * :param session_sftp: sftp_session object.
 * :param abs_path_local: Absolute path of the file on local machine.
 * :param abs_path_remote: Absolute path of the file on remote machine.
 */
static CommandStatusE
copy_remote_dir_recursively(ssh_session session_ssh, sftp_session session_sftp,
                            char *abs_path_remote, char *abs_path_local) {
    ListT *sub_dir_path_stack = List_new(1, sizeof(char *));
    ListT *remote_dir;
    FileSystemT *filesystem;
    char *dir_path_remote = NULL;
    char *dir_path_local = NULL;
    char *file_path_local = NULL;

    do {
        if (dir_path_remote == NULL || dir_path_local == NULL) {
            dir_path_remote = abs_path_remote;
            dir_path_local = abs_path_local;
        } else {
            dir_path_remote = List_pop(sub_dir_path_stack);
            dir_path_local = dir_path_remote;
            path_replace(dir_path_local, abs_path_remote, abs_path_local, 1);
        }

        path_mkdir_parents(dir_path_local, strlen(dir_path_local));
        remote_dir = path_read_remote_dir(session_ssh, session_sftp, dir_path_remote);
        if (remote_dir == NULL) {
            return CMD_INTERNAL_ERROR;
        }

        for (size_t i = 0; i < remote_dir->length; i++) {
            filesystem = List_get(remote_dir, i);

            puts(filesystem->name);
            if (path_is_dotted(filesystem->name, strlen(filesystem->name))) {
                continue;
            }

            switch (filesystem->type) {
                case FS_REG_FILE:
                    file_path_local = strdup(filesystem->relative_path);
                    path_replace(file_path_local, abs_path_remote, abs_path_local, 1);
                    copy_from_remote_to_local(session_ssh, session_sftp,
                                              filesystem->relative_path, file_path_local);

                    break;
                case FS_DIRECTORY:
                    List_push(sub_dir_path_stack, filesystem->relative_path,
                              (sizeof *filesystem->relative_path) *
                                  (strlen(filesystem->relative_path) + 1));
                    break;
                case FS_SYM_LINK:
                    break; /* TODO */
                default:
                    DBG_ERR("Unknown type %d", filesystem->type);
            }
        }

        dbg_safe_free(file_path_local);

    } while (!List_is_empty(sub_dir_path_stack));

    return CMD_OK;
}

static CommandStatusE
copy_local_dir_recursively(ssh_session session_ssh, sftp_session session_sftp,
                           char *abs_path_local, char *abs_path_remote) {
    ListT *sub_dir_path_stack = List_new(1, sizeof(char *));
    ListT *local_dir;
    FileSystemT *filesystem;
    char *dir_path_remote = NULL;
    char *dir_path_local = NULL;
    char *file_path_remote = NULL;

    do {
        if (dir_path_remote == NULL || dir_path_local == NULL) {
            dir_path_remote = abs_path_remote;
            dir_path_local = abs_path_local;
        } else {
            dir_path_local = List_pop(sub_dir_path_stack);
            dir_path_remote = strdup(dir_path_local);
            path_replace(dir_path_remote, abs_path_local, abs_path_remote, 1);
        }

        create_parents_remote(session_ssh, session_sftp, dir_path_remote);
        local_dir = path_read_local_dir(dir_path_local);
        if (local_dir == NULL) {
            return CMD_INTERNAL_ERROR;
        }

        for (size_t i = 0; i < local_dir->length; i++) {
            filesystem = List_get(local_dir, i);

            if (path_is_dotted(filesystem->name, strlen(filesystem->name))) {
                continue;
            }

            switch (filesystem->type) {
                case FS_REG_FILE:
                    file_path_remote = strdup(filesystem->relative_path);
                    path_replace(file_path_remote, abs_path_local, abs_path_remote, 1);
                    copy_from_local_to_remote(session_ssh, session_sftp,
                                              filesystem->relative_path,
                                              file_path_remote);

                    break;
                case FS_DIRECTORY:
                    List_push(sub_dir_path_stack, filesystem->relative_path,
                              (sizeof *filesystem->relative_path) *
                                  (strlen(filesystem->relative_path) + 1));
                    break;
                case FS_SYM_LINK:
                    break; /* TODO */
                default:
                    DBG_ERR("Unknown type %d", filesystem->type);
            }
        }
        dbg_safe_free(file_path_remote);

    } while (!List_is_empty(sub_dir_path_stack));

    return CMD_OK;
}

/**
 * Helper function to copy a file from remote to local server.
 *
 * :param session_ssh: ssh_session object.
 * :param session_sftp: sftp_session object.
 * :param abs_path_local: Absolute path of the file on local machine.
 * :param abs_path_remote: Absolute path of the file on remote machine.
 */
CommandStatusE
copy_from_remote_to_local(ssh_session session_ssh, sftp_session session_sftp,
                          char *abs_path_remote, char *abs_path_local) {
    sftp_attributes from = sftp_stat(session_sftp, abs_path_remote);

    if (from == NULL) {
        DBG_ERR("Failed to get attributes for %s: %s", abs_path_remote,
                ssh_get_error(session_ssh));
        return CMD_INTERNAL_ERROR;
    }

    if (from->type == SSH_FILEXFER_TYPE_DIRECTORY) {
        DBG_DEBUG("Copying dir from %s to %s", abs_path_remote, abs_path_local);
        return copy_remote_dir_recursively(session_ssh, session_sftp, abs_path_remote,
                                           abs_path_local);
    } else if (from->type == SSH_FILEXFER_TYPE_REGULAR) {
        DBG_DEBUG("Copying file from %s to %s", abs_path_remote, abs_path_local);
        return copy_file_from_remote_to_local(session_ssh, session_sftp, abs_path_remote,
                                              abs_path_local);
    }

    return CMD_OK;
}

CommandStatusE
copy_from_local_to_remote(ssh_session session_ssh, sftp_session session_sftp,
                          char *abs_path_local, char *abs_path_remote) {
    struct stat from;
    stat(abs_path_local, &from);

    if (S_ISDIR(from.st_mode)) {
        DBG_DEBUG("Copying dir from %s to %s", abs_path_local, abs_path_remote);
        return copy_local_dir_recursively(session_ssh, session_sftp, abs_path_local,
                                          abs_path_remote);
    } else if (S_ISREG(from.st_mode)) {
        DBG_DEBUG("Copying file from %s to %s", abs_path_local, abs_path_remote);
        return copy_file_from_local_to_remote(session_ssh, session_sftp, abs_path_local,
                                              abs_path_remote);
    }

    return CMD_OK;
}

/**
 * Helper function to free the ssh session and its resources.
 * If the session is connected, it will be disconnect safely.
 */
void
clean_ssh_session(ssh_session session) {
    DBG_DEBUG("Freeing session: %p", (void *)session);

    if (ssh_is_connected(session)) {
        ssh_disconnect(session);
    }
    ssh_free(session);
    ssh_finalize();
}

/** Helper function to free the sftp session and its resources. */
void
clean_sftp_session(sftp_session session) {
    DBG_DEBUG("Freeing session: %p", (void *)session);

    sftp_free(session);
}
