#ifndef SFTP_PATH_H
#define SFTP_PATH_H

#include <stdint.h>
#include <stdlib.h>
#include "sftp_list.h"

#define BUF_SIZE_FS_NAME 128
#define BUF_SIZE_FS_PATH 2048
#define BUF_SIZE_FILE_CONTENTS 65536

#define BUF_SIZE_FS_NAME 128
#define BUF_SIZE_FS_PATH 2048

#define __NUM_ARGS(type, ...) (sizeof((type[]){__VA_ARGS__}) / sizeof(type))
#define FS_JOIN_PATH(...) path_join(__NUM_ARGS(char *, __VA_ARGS__), __VA_ARGS__)

#ifdef WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

char *path_str_slice(char *path_str, size_t start, size_t stop);
char *path_remove_prefix(char *path_str, size_t length);
char *path_remove_suffix(char *path_str, size_t length);
char *path_join(size_t num_paths, ...);
uint8_t path_mkdir_parents(char *path_str, size_t length);
ListT *path_split(char *path_str, size_t length);

#endif /* ifndef SFTP_PATH_H */
