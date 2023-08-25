#ifndef SFTP_PATH_H
#define SFTP_PATH_H

#include <stdint.h>
#include <stdlib.h>
#include "sftp_list.h"

#define BUF_SIZE_FS_NAME 128
#define BUF_SIZE_FS_PATH 2048
#define BUF_SIZE_FILE_CONTENTS 16384

/** SSH does not impose any restrictions on the passphrase length,
 * but for simplicity, we will have a finite length passphrase buffer. */
#define BUF_SIZE_PASSPHRASE 128

#define BUF_SIZE_FS_NAME 128
#define BUF_SIZE_FS_PATH 2048

/* File owner has perms to Read, Write and Execute the rest can only Read and Execute */
#define FS_CREATE_PERM (S_IRWXU | S_IRWXG | S_IRWXO)

/* Simple macro to count the number of arguments in ``__VA_ARGS__`` */
#define __NUM_ARGS(type, ...) (sizeof((type[]){__VA_ARGS__}) / sizeof(type))

/** Macro to join paths using ``PATH_SEPARATOR`` */
#define FS_JOIN_PATH(path_buf, ...) \
    path_join(path_buf, __NUM_ARGS(char *, __VA_ARGS__), __VA_ARGS__)

/** Set ``PATH_SEPARATOR`` to ``\\`` on Windows and ``/`` on Posix-systems */
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

/** Structure to hold information about a file system object */
typedef struct {
    /** File name */
    char *name;

    /** Relative path to the file system object */
    char *relative_path;

    /** Type of the file system object */
    FileTypesT type;
} FileSystemT;

char *path_str_slice(const char *path_str, size_t start, size_t stop);
void path_remove_prefix(char *path_str);
void path_remove_suffix(char *path_str);
void path_join(char *path_buf, size_t num_paths, ...);
bool path_is_dotted(const char *path_str, size_t length);
bool path_is_hidden(const char *path_str, size_t length);
uint8_t path_mkdir_parents(char *path_str, size_t length);
ListT *path_split(const char *path_str, size_t length);
void path_replace_grandparent(char *path_str, char *grandparent);
void path_replace(char *path_str, char *path_head_to_replace, char *path_head_replacement,
                  size_t max_count);
ListT *path_read_local_dir(char *dir_path);
ListT *path_read_remote_dir(ssh_session session_ssh, sftp_session session_sftp,
                            char *dir_path);

void FileSystem_free(FileSystemT *self);
void FileSystem_list_free(ListT *self);

#endif /* ifndef SFTP_PATH_H */
