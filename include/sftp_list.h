#ifndef SFTP_LIST_H
#define SFTP_LIST_H

#include <stdbool.h>

#include <libssh/sftp.h>

typedef struct {
    /** Array of directories pushed to the list
     *
     * .. note::
     *
     *    The type of the attribute should always be ``SSH_FILEXFER_TYPE_DIRECTORY`` */
    void **list;

    /** Length of the array */
    size_t length;

    /** Size allocated for the array */
    size_t allocated;
} ListT;

ListT *List_new(size_t length, size_t type_size);
void List_push(ListT *self, void *attr);
void *List_pop(ListT *self);
bool List_is_empty(ListT *self);
void List_free(ListT *self);

#endif /* SFTP_LIST_H */
