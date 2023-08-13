#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "commands.h"
#include "debug.h"
#include "sftp_list.h"
#include "sftp_path.h"

char *
path_str_slice(const char *path_str, size_t start, size_t stop) {
    size_t length = stop - start;
    char *sliced_path_str;

    if (length < 1) {
        return NULL;
    }

    sliced_path_str = calloc(length, sizeof *sliced_path_str);
    for (size_t i = start; i < stop; i++) {
        sliced_path_str[i - start] = path_str[i];
    }
    sliced_path_str[stop - 1] = '\0';

    return sliced_path_str;
}

/**
 * Remove redundant prefixes from filesystem items (files and dirs).
 * Basically converts ``./this`` to ``this`` and ``////this`` to ``/this``
 */
char *
path_remove_prefix(char *path_str, size_t length) {
    char *cleaned_path_str = path_str;
    size_t num_slash_prefix = 0;
    uint8_t is_root_dir = 1;

    if (length < 2) {
        return path_str;
    }

    if (path_str[0] == '.' && path_str[1] == PATH_SEPARATOR) {
        cleaned_path_str = path_str_slice(path_str, 2, length);
        length -= 2;
        is_root_dir = 0;
    }

    for (size_t i = 0; i < length; i++) {
        /* Break as soon as the first non path separator is found. */
        if (cleaned_path_str[i] != PATH_SEPARATOR) {
            break;
        }

        num_slash_prefix++;
    }

    if (num_slash_prefix) {
        cleaned_path_str =
            path_str_slice(cleaned_path_str, num_slash_prefix - is_root_dir, length);
    }

    return cleaned_path_str;
}

char *
path_remove_suffix(char *path_str, size_t length) {
    char *cleaned_path_str = path_str;
    size_t num_slash_suffix = 0;

    if (length < 2) {
        return cleaned_path_str;
    }

    for (ssize_t i = (ssize_t)length - 1; i > -1; i++) {
        if (cleaned_path_str[i] != PATH_SEPARATOR) {
            break;
        }
        num_slash_suffix++;
    }

    if (num_slash_suffix) {
        cleaned_path_str = path_str_slice(path_str, 0, length - num_slash_suffix + 1);
    }

    return cleaned_path_str;
}

char *
path_join(size_t num_paths, ...) {
    va_list args;
    size_t fs_len = 0;
    size_t path_len = 0;
    char *path_buf = calloc(BUF_SIZE_FS_PATH, (sizeof *path_buf));
    char *fs_name;

    va_start(args, num_paths);
    for (size_t i = 0; i < num_paths && path_len < BUF_SIZE_FS_PATH; i++) {
        fs_name = va_arg(args, char *);
        fs_len = strlen(fs_name);
        fs_name = path_remove_prefix(fs_name, fs_len);
        fs_len = strlen(fs_name);
        fs_name = path_remove_suffix(fs_name, fs_len);

        path_len += strlen(fs_name);
        strcat(path_buf, fs_name);

        if (i < num_paths - 1) {
            path_buf[path_len++] = PATH_SEPARATOR;
        }
    }
    va_end(args);

    return path_buf;
}

void
path_buf_clear(char *path_buf, size_t length) {
    for (size_t i = 0; i < length; i++) {
        path_buf[i] = '\0';
    }
}

ListT *
path_split(const char *path_str, size_t length) {
    ListT *path_list = List_new(0, sizeof(char *));
    char *path_buf = malloc(BUF_SIZE_FS_NAME * sizeof *path_buf);
    size_t path_buf_len = 0;

    for (size_t i = 0; i < length; i++) {
        if (path_str[i] != PATH_SEPARATOR) {
            path_buf[path_buf_len++] = path_str[i];
            continue;
        }
        path_buf[path_buf_len] = '\0';
        List_push(path_list, path_buf);
        path_buf_clear(path_buf, BUF_SIZE_FS_NAME);
        path_buf_len = 0;
    }
    List_push(path_list, path_buf);

    return path_list;
}

bool
path_is_dotted(const char *path_str, size_t length) {
    if (length > 2) {
        return false;
    }
    return !strcmp(path_str, ".") || !strcmp(path_str, "..");
}

char *
path_replace_grand_parent(char *path_str, size_t length_str, char *grand_parent) {
    size_t slash_index = 0;
    char *replaced_head = path_str;

    if (length_str < 3) {
        return false;
    }

    for (; slash_index < length_str; slash_index++) {
        if (path_str[slash_index] == PATH_SEPARATOR) {
            break;
        }
    }

    if (slash_index == length_str) {
        return replaced_head;
    }

    replaced_head = path_str_slice(replaced_head, slash_index + 1, length_str);
    replaced_head = FS_JOIN_PATH(grand_parent, replaced_head);
    puts(replaced_head);

    return replaced_head;
}

uint8_t
path_mkdir_parents(char *path_str, size_t length) {
    char *path_buf = malloc(BUF_SIZE_FS_PATH * sizeof *path_buf);

    if (length < 1) {
        return 0;
    }
    /* TODO: implement the function, avoid the system-call */
    snprintf(path_buf, BUF_SIZE_FS_PATH, "mkdir -p %s", path_str);
    system(path_buf);
    return 1;
}

FileSystemT *
FileSystem_from_path(char *path, uint8_t type) {
    FileSystemT *file_system = malloc(sizeof *file_system);
    ListT *split;
    file_system->type = type;

    if (*path == PATH_SEPARATOR) {
        file_system->absoulte_path = path;
    } else {
        file_system->relative_path = path;
    }

    split = path_split(path, strlen(path));
    file_system->name = List_pop(split);

    return file_system;
}

ListT *
path_read_remote_dir(ssh_session session_ssh, sftp_session session_sftp, char *path) {
    sftp_dir dir;
    uint8_t result;
    sftp_attributes attr;
    FileTypesT file_system_type;
    char *attr_relative_path;
    ListT *path_content_list = List_new(1, sizeof(FileSystemT *));

    dir = sftp_opendir(session_sftp, path);
    if (dir == NULL) {
        DBG_ERR("Couldn't open remote directory `%s`: %s\n", path,
                ssh_get_error(session_ssh));
        return NULL;
    }

    while ((attr = sftp_readdir(session_sftp, dir)) != NULL) {
        attr_relative_path = FS_JOIN_PATH(path, attr->name);

        switch (attr->type) {
            case SSH_FILEXFER_TYPE_REGULAR:
                file_system_type = FS_REG_FILE;
                break;
            case SSH_FILEXFER_TYPE_DIRECTORY:
                file_system_type = FS_DIRECTORY;
                break;
            default:
                DBG_INFO("Ignoring filetype %d\n", attr->type);
        }
        List_push(path_content_list,
                  FileSystem_from_path(attr_relative_path, file_system_type));
    }

    result = sftp_closedir(dir);
    if (result != SSH_FX_OK) {
        DBG_ERR("Couldn't close directory %s: %s\n", dir->name,
                ssh_get_error(session_ssh));
        return NULL;
    }

    return path_content_list;
}
