#ifndef ATTR_LIST_H
#define ATTR_LIST_H

#include <stdbool.h>

#include <libssh/sftp.h>

typedef struct {
    /** Array of directories pushed to the list
     *
     * .. note::
     *
     *    The type of the attribute should always be ``SSH_FILEXFER_TYPE_DIRECTORY`` */
    sftp_attributes *dirs;

    /** Length of the array */
    size_t length;

    /** Size allocated for the array */
    size_t allocated;
} AttrListT;

AttrListT *AttrList_new(size_t length);
void AttrList_push(AttrListT *self, sftp_attributes attr);
sftp_attributes AttrList_pop(AttrListT *self);
bool AttrList_is_empty(AttrListT *self);
void AttrList_free(AttrListT *self);

#endif
