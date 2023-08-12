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

/* File owner has perms to Read, Write and Execute the rest can only Read and Execute */
#define FS_CREATE_PERM (S_IRWXU | S_IRWXG | S_IRWXO)

/* Simple macro to count the number of arguments in ``__VA_ARGS__`` */
#define __NUM_ARGS(type, ...) (sizeof((type[]){__VA_ARGS__}) / sizeof(type))

#define FS_JOIN_PATH(...) path_join(__NUM_ARGS(char *, __VA_ARGS__), __VA_ARGS__)

#ifdef WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

typedef enum {
    FS_REG_FILE = 1,
    FS_DIRECTORY = 2,
    FS_SYM_LINK = 3,
} FileTypesT;

typedef struct {
    char *name;
    char *absoulte_path;
    char *grand_parent_path;
    char *parent_path;
    char *relative_path;
    FileTypesT type;
} FileSystemT;

char *path_str_slice(const char *path_str, size_t start, size_t stop);
char *path_remove_prefix(char *path_str, size_t length);
char *path_remove_suffix(char *path_str, size_t length);
char *path_join(size_t num_paths, ...);
bool path_is_dotted(const char *path_str, size_t length);
uint8_t path_mkdir_parents(char *path_str, size_t length);
ListT *path_split(const char *path_str, size_t length);
char *path_replace_grand_parent(char *path_str, size_t length_str, char *grand_parent);
ListT *path_read_local_dir(char *dir_path);
ListT *path_read_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                            char *dir_path);

#endif /* ifndef SFTP_PATH_H */
