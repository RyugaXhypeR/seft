#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <dirent.h>

#include "commands.h"
#include "debug.h"
#include "sftp_list.h"
#include "sftp_path.h"

/**
 * Split a path string into a list of path components and return the sliced string.
 *
 * :param path_str:  NULL terminated string representing a path. The path can be absolute
 * or relative. :param start: [INCLUSIVE] Start index of the slice. It must be less than
 * stop and length of ``path_str``. :param stop: [EXCLUSIVE] Stop index of the slice. It
 * must be greater than start. :return: Sliced path string. It must be freed by the
 * caller.
 */
char *
path_str_slice(const char *path_str, size_t start, size_t stop) {
    size_t length = stop - start;
    char *sliced_path_str;

    if (length < 1) {
        return NULL;
    }

    sliced_path_str = dbg_calloc(length, sizeof *sliced_path_str);
    for (size_t i = start; i < stop; i++) {
        sliced_path_str[i - start] = path_str[i];
    }
    sliced_path_str[stop - 1] = '\0';

    return sliced_path_str;
}

void
path_str_shift_left(char *path_str, size_t num_shifts) {
    size_t len_path_str = strlen(path_str);

    if (num_shifts > len_path_str) {
        return;
    }

    for (size_t i = 0; i < num_shifts; i++, len_path_str--) {
        for (size_t j = 0; j < len_path_str; j++) {
            path_str[j] = path_str[j + 1];
        }
    }
    path_str[len_path_str] = '\0';
}

/**
 * Remove redundant prefixes from filesystem items (files and dirs).
 * Basically converts ``./this`` to ``this`` and ``////this`` to ``/this``
 */
void
path_remove_prefix(char *path_str) {
    size_t num_slash_prefix = 0;
    int8_t root_dir_slash = 1;
    size_t len_path_str = strlen(path_str);

    if (len_path_str < 2) {
        return;
    }

    /* Checking is path starts with ``./`` */
    if (path_str[0] == '.' && path_str[1] == PATH_SEPARATOR) {
        path_str_shift_left(path_str, 2);
        len_path_str -= 2;
        root_dir_slash = 0;
    }

    for (size_t i = 0; i < len_path_str; i++, num_slash_prefix++) {
        /* Break as soon as the first non path separator is found. */
        if (path_str[i] != PATH_SEPARATOR) {
            break;
        }
    }

    if (num_slash_prefix) {
        path_str_shift_left(path_str, num_slash_prefix - root_dir_slash);
    }
}

/**
 * Remove redundant suffixes from filesystem items (files and dirs).
 * For example: ``this/`` to ``this`` and ``this////`` to ``this``
 */
void
path_remove_suffix(char *path_str) {
    size_t num_slash_suffix = 0;
    size_t len_path_str = strlen(path_str);

    if (len_path_str < 2) {
        return;
    }

    for (ssize_t i = (ssize_t)len_path_str - 1; i > -1; i--, num_slash_suffix++) {
        if (path_str[i] != PATH_SEPARATOR) {
            break;
        }
    }

    path_str[len_path_str - num_slash_suffix] = '\0';
}

/**
 * Join multiple paths into a single path.
 *
 * :param num_paths: Number of paths to join.
 * :param ...: Variable number of paths to join.
 * :return: Joined path string. It must be freed by the caller.
 *
 * .. note:: Recommended to use ``FS_PATH_JOIN(...)` instead of this function.
 */
void
path_join(char *path_buf, size_t num_paths, ...) {
    va_list args;
    size_t path_len = strlen(path_buf);
    char *fs_name;

    va_start(args, num_paths);
    for (size_t i = 0; i < num_paths && path_len < BUF_SIZE_FS_PATH; i++) {
        fs_name = va_arg(args, char *);
        path_remove_prefix(fs_name);
        path_remove_suffix(fs_name);

        if (path_len) {
            path_buf[path_len++] = PATH_SEPARATOR;
        }

        path_len += strlen(fs_name);
        strcat(path_buf, fs_name);
    }
    va_end(args);
}

/** Clear a path buffer. */
void
path_buf_clear(char *path_buf, size_t length) {
    for (size_t i = 0; i < length; i++) {
        path_buf[i] = '\0';
    }
}

/**
 * Split a path string into a list of path components.
 * For example: ``/this/is/a/path`` to ``["this", "is", "a", "path"]``
 *
 * :param path_str:  NULL terminated string representing a path. The path can be
 * absolute or relative. :param length: Length of the path string. :return: List of
 * path components. It must be freed by the caller.
 */
ListT *
path_split(const char *path_str, size_t length) {
    ListT *path_list = List_new(0, sizeof(char *));
    char *path_buf = dbg_calloc(BUF_SIZE_FS_NAME,  sizeof *path_buf);
    size_t path_buf_len = 0;

    for (size_t i = 0; i < length; i++) {
        if (path_str[i] != PATH_SEPARATOR) {
            path_buf[path_buf_len++] = path_str[i];
            continue;
        }
        path_buf[path_buf_len] = '\0';
        List_push(path_list, path_buf, path_buf_len + 1);
        path_buf_clear(path_buf, BUF_SIZE_FS_NAME);
        path_buf_len = 0;
    }
    List_push(path_list, path_buf, path_buf_len + 1);
    free(path_buf);

    return path_list;
}

/**
 * Check if a path is dotted.
 * For example: ``.`` and ``..`` are dotted paths.
 *
 * :param path_str:  NULL terminated string representing a path. The path can be
 * absolute or relative. :param length: Length of the path string. :return: True if
 * the path is dotted, False otherwise.
 */
bool
path_is_dotted(const char *path_str, size_t length) {
    if (length > 2) {
        return false;
    }
    return !strcmp(path_str, ".") || !strcmp(path_str, "..");
}

/**
 * Check if a path is hidden.
 * For example: ``.hidden`` and ``.hidden/`` are hidden paths.
 *
 * :param path_str:  NULL terminated string representing a path. The path can be
 * absolute or relative. :param length: Length of the path string. :return: True if
 * the path is hidden, False otherwise.
 */
bool
path_is_hidden(const char *path_str, size_t length) {
    if (!length) {
        return false;
    }

    return *path_str == '.';
}

/**
 * Replace the head (grandparent) of a path with a new head.
 * For example: ``/this/is/a/path`` to ``/new/head/is/a/path``
 *
 * :param path_str:  NULL terminated string representing a path. The path can be
 * absolute or relative. :param length: Length of the path string. :param grandparent:
 * New head of the path.
 */
void
path_replace_grandparent(char *path_str, char *grandparent) {
    size_t slash_index = 0;
    char *replaced_head = path_str;
    size_t len_path_str = strlen(path_str);

    if (len_path_str < 3) {
        return;
    }

    for (; slash_index < len_path_str; slash_index++) {
        if (path_str[slash_index] == PATH_SEPARATOR) {
            break;
        }
    }

    if (slash_index == len_path_str) {
        return;
        ;
    }

    path_str_shift_left(replaced_head, slash_index + 1);
    path_buf_clear(path_str, strlen(path_str));
    FS_JOIN_PATH(path_str, grandparent, replaced_head);
    dbg_safe_free(replaced_head);
}

void
path_replace(char *path_str, char *path_head_to_replace, char *path_head_replacement,
             size_t max_count) {
    size_t len_path_head_to_replace = strlen(path_head_to_replace);
    size_t len_path_head_replacement = strlen(path_head_replacement);
    size_t remaining_len;
    char *occurrence;

    while (max_count--) {
        /* ``occurrence`` is a reference to first occurrence of ``path_head_to_replace``
         * in ``path_str``. No need to free ``occurrence`` */
        occurrence = strstr(path_str, path_head_to_replace);
        if (occurrence == NULL) {
            break;
        }

        remaining_len = strlen(occurrence + len_path_head_to_replace);
        memmove(occurrence + len_path_head_replacement,
                occurrence + len_path_head_to_replace, remaining_len + 1);
        memcpy(occurrence, path_head_replacement, len_path_head_replacement);
        path_str = occurrence + len_path_head_replacement;
    }
}

/** Create parent directories of the given path if they don't exist. */
uint8_t
path_mkdir_parents(char *path_str, size_t length) {
    struct stat _dir_stats;
    int8_t result;
    char *path_buf;
    ListT *path_list = path_split(path_str, length);
    path_buf = List_get(path_list, 0);

    for (size_t i = 0; i < path_list->length; i++) {
        if (i) {
            FS_JOIN_PATH(path_buf, List_get(path_list, i));
        }

        if (stat(path_buf, &_dir_stats) == -1) {
            /* Directory does not exist */

            if ((result = mkdir(path_buf, FS_CREATE_PERM))) {
                DBG_ERR("Couldn't create directory %s: Error code: %d\n", path_buf,
                        result);
                return 0;
            }
        }
    }

    dbg_safe_free(path_buf);
    List_free(path_list);

    return 1;
}

/** Create a new file system object from a path. */
FileSystemT *
FileSystem_from_path(char *path, uint8_t type) {
    FileSystemT *file_system = dbg_malloc(sizeof *file_system);
    ListT *split;

    *file_system = (FileSystemT){
        .name = dbg_malloc(BUF_SIZE_FS_NAME),
        .relative_path = dbg_malloc(BUF_SIZE_FS_PATH),
        .type = type,
    };

    strcpy(file_system->relative_path, path);

    split = path_split(path, strlen(path));
    strcpy(file_system->name, List_pop(split));

    for (size_t i = 0; i < split->length - 1; i++) {
        dbg_safe_free(List_pop(split));
    }

    return file_system;
}

/**
 * Free a file system object.
 *
 * .. note:: Deep frees all attributes, so any references to them will be invalid.
 * */
void
FileSystem_free(FileSystemT *self) {
    dbg_safe_free(self->name);
    dbg_safe_free(self->relative_path);
    dbg_safe_free(self);
}

void
FileSystem_list_free(ListT *self) {
    for (size_t i = 0; i < List_length(self); i++) {
        FileSystem_free(List_get(self, i));
    }

    dbg_safe_free(self);
}

void
FileSystem_copy(FileSystemT *self, FileSystemT *dest) {
    strcpy(dest->name, self->name);
    strcpy(dest->relative_path, self->relative_path);
}

void
FileSystem_list_push(ListT *self, FileSystemT *fs) {
    List_realloc(self, self->length + 1);
    self->list[self->length] = FileSystem_from_path(fs->relative_path, fs->type);
    FileSystem_copy(fs, self->list[self->length++]);
}

/**
 * Read the contents of a local directory and return a list of file system objects.
 *
 * :param path: Path to the directory.
 * :return: List of file system objects.
 */
ListT *
path_read_remote_dir(ssh_session session_ssh, sftp_session session_sftp, char *path) {
    sftp_dir dir;
    uint8_t result;
    sftp_attributes attr;
    FileTypesT filesystem_type;
    FileSystemT *filesystem;
    char *attr_relative_path = dbg_calloc(BUF_SIZE_FS_PATH, sizeof *attr_relative_path);
    ListT *path_content_list = List_new(1, sizeof(FileSystemT *));

    dir = sftp_opendir(session_sftp, path);
    if (dir == NULL) {
        DBG_ERR("Couldn't open remote directory `%s`: %s\n", path,
                ssh_get_error(session_ssh));
        return NULL;
    }

    while ((attr = sftp_readdir(session_sftp, dir)) != NULL) {
        FS_JOIN_PATH(attr_relative_path, path, attr->name);

        switch (attr->type) {
            case SSH_FILEXFER_TYPE_REGULAR:
                filesystem_type = FS_REG_FILE;
                break;
            case SSH_FILEXFER_TYPE_DIRECTORY:
                filesystem_type = FS_DIRECTORY;
                break;
            default:
                DBG_INFO("Ignoring filetype %d\n", attr->type);
                continue;
        }
        filesystem = FileSystem_from_path(attr_relative_path, filesystem_type);
        path_buf_clear(attr_relative_path, strlen(attr_relative_path));
        FileSystem_list_push(path_content_list, filesystem);
        FileSystem_free(filesystem);
    }

    dbg_safe_free(attr_relative_path);

    result = sftp_closedir(dir);
    if (result != SSH_FX_OK) {
        DBG_ERR("Couldn't close directory %s: %s\n", dir->name,
                ssh_get_error(session_ssh));
        return NULL;
    }

    return path_content_list;
}

ListT *
path_read_local_dir(char *path) {
    DIR *dir;
    uint32_t result;
    struct dirent *attr;
    FileSystemT *filesystem;
    FileTypesT filesystem_type;
    char *attr_relative_path = dbg_calloc(BUF_SIZE_FS_PATH, sizeof *attr_relative_path);
    ListT *path_content_list = List_new(1, sizeof(FileSystemT *));

    dir = opendir(path);
    if (dir == NULL) {
        DBG_ERR("Couldn't open local directory `%s`", path);
        return NULL;
    }

    while ((attr = readdir(dir)) != NULL) {
        FS_JOIN_PATH(attr_relative_path, path, attr->d_name);

        switch (attr->d_type) {
            case DT_REG:
                filesystem_type = FS_REG_FILE;
                break;
            case DT_DIR:
                filesystem_type = FS_DIRECTORY;
                break;
            default:
                DBG_INFO("Ignoring filetype %d\n", attr->d_type);
                continue;
        }
        filesystem = FileSystem_from_path(attr_relative_path, filesystem_type);
        path_buf_clear(attr_relative_path, strlen(attr_relative_path) + 1);
        FileSystem_list_push(path_content_list, filesystem);
        FileSystem_free(filesystem);
    }

    dbg_safe_free(attr_relative_path);

    result = closedir(dir);
    if (result) {
        DBG_ERR("Couldn't close directory %s", path);
        return NULL;
    }

    return path_content_list;
}
