#include <stdlib.h>

#include <libssh/sftp.h>

#include "attr_list.h"
#include "debug.h"

AttrListT *
AttrList_new(size_t length) {
    AttrListT *self = malloc(sizeof *self);
    sftp_attributes *dirs = malloc(length * (sizeof *dirs));
    *self = (AttrListT){.dirs = dirs, .length = length, .allocated = length};
    return self;
}

void
AttrList_re_alloc(AttrListT *self, size_t new_size) {
    if (self->allocated >= new_size) {
        return;
    }

    /** Over-allocating the list to reduce calls to ``realloc``
     *
     * .. note:: For more information about the algrithm check
     *    `PyList resized-length algorithm`_.
     *
     * .. PyList resized-length algorithm::
     *
     *      https://github.com/python/cpython/blob/main/Objects/listobject.c#L62-#L72 */
    new_size = (new_size + (new_size >> 3) + 6) & ~(size_t)3;

    self->dirs = realloc(self->dirs, new_size);
    if (self->dirs == NULL) {
        DBG_ERR(
            "Couldn't reallocate memory for `AttrListT.dirs`, tried to allocate %zu size",
            new_size);
        return;
    }

    self->allocated = new_size;
}

void
AttrList_push(AttrListT *self, sftp_attributes attr) {
    AttrList_re_alloc(self, self->length + 1);
    self->dirs[self->length++] = attr;
}

bool
AttrList_is_empty(AttrListT *self) {
    return !self->length;
}

void AttrList_free(AttrListT *self) {
    free(self->dirs);
    free(self);
}
