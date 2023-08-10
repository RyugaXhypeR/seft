#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sftp_list.h"
#include "sftp_path.h"

char *
path_str_slice(char *path_str, size_t start, size_t stop) {
    size_t length = stop - start;
    char *sliced_path_str;

    if (length < 1) {
        return NULL;
    }

    sliced_path_str = malloc(length * sizeof *sliced_path_str);
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
        /* Break as soon as the first non path seperator is found. */
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

    for (ssize_t i = length - 1; i > -1; i++) {
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
    char *path_buf = malloc(BUF_SIZE_FS_PATH * (sizeof *path_buf));
    char *fs_name = malloc(BUF_SIZE_FS_NAME * (sizeof *path_buf));

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

ListT *
path_split(char *path_str, size_t length) {
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
        path_buf_len = 0;
    }
    List_push(path_list, path_buf);

    return path_list;
}

uint8_t
path_mkdir_parents(char *path_str, size_t length) {
    puts(path_str);
    struct stat _dir_stats;
    char *path_buf = malloc(BUF_SIZE_FS_NAME * sizeof *path_buf);
    ListT *path_list = path_split(path_str, length);
    path_buf = List_get(path_list, 0);

    for (size_t i = 0; i < path_list->length; i++) {
        if (i) {
            path_buf = FS_JOIN_PATH(path_buf, List_get(path_list, i));
        }

        if (stat(path_buf, &_dir_stats) == -1) {
            if (!mkdir(path_buf, 0700)) {
                perror("mkdir");
            }
        }
    }

    List_free(path_list);
    return 1;
}
