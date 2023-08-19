#include <stdlib.h>

#include <libssh/sftp.h>
#include <string.h>

#include "sftp_list.h"
#include "debug.h"

ListT *
List_new(size_t length, size_t type_size) {
    ListT *self = malloc(sizeof *self);
    void **list = malloc(length * type_size);
    *self = (ListT){.list = list, .length = 0, .allocated = length};
    return self;
}

/** Reallocate memory for ``ListT``.
 *
 * .. note:: This function will only allocate memory if ``new_size`` is
 *    greater than ``self->allocated``, so its safe to call it without
 *    checking ``self->allocated``.
 */
void
List_re_alloc(ListT *self, size_t new_size) {
    if (self->allocated >= new_size) {
        return;
    }

    /** Over-allocating the list to reduce calls to ``realloc``
     *
     * .. note:: For more information about the algorithm check
     *    `PyList resized-length algorithm`_.
     *
     * .. PyList resized-length algorithm::
     *
     *      https://github.com/python/cpython/blob/main/Objects/listobject.c#L62-#L72 */
    new_size = (new_size + (new_size >> 3) + 6) & ~(size_t)3;

    self->list = realloc(self->list, new_size * sizeof *self->list);
    if (self->list == NULL) {
        DBG_ERR("Couldn't reallocate memory for `ListT.dirs`, tried to allocate %zu size",
                new_size);
        return;
    }

    self->allocated = new_size;
}

/**
 * Push an item to the end of the list.
 *
 * :param self: The list to push to
 * :param other: The item to push
 * :param size: The size of elements to push and the size of the type of the element
 *
 * .. note:: Calls ``memcpy`` to copy ``other`` to the end of the list.
 */
void
List_push(ListT *self, void *other, size_t size) {
    List_re_alloc(self, self->length + 1);
    self->list[self->length] = malloc(size);
    memcpy(self->list[self->length++], other, size);
}

/**
 * Pop an item from the end of the list.
 *
 * :param self: The list to pop from
 *
 * :returns: The popped item
 */
void *
List_pop(ListT *self) {
    if (!List_is_empty(self)) {
        return self->list[--self->length];
    }
    return NULL;
}

/** Check if the list is empty. */
bool
List_is_empty(ListT *self) {
    return !self->length;
}

/** Get the ``i``th index of the list */
void *
List_get(ListT *self, size_t index) {
    if (index >= self->length) {
        return NULL;
    }

    return self->list[index];
}

size_t
List_length(ListT *self) {
    return self->length;
}

/** Free the list and its items. */
void
List_free(ListT *self) {
    free(self->list);
    free(self);
}

/**
 * Slice the list from ``start`` to ``stop``. This is generalized for pointers,
 * so if any complex data types like ``struct`` are used, they must be handled
 * by the user.
 *
 * :param self: The list to slice
 * :param start: [INCLUSIVE] The start index
 * :param stop: [EXCLUSIVE] The stop index
 *
 * .. note:: If ``stop`` is greater than the length of the list, it will be
 *    set to the length of the list.
 */
ListT *
List_slice(ListT *self, size_t start, size_t stop) {
    size_t length = stop - start;
    ListT *sliced_list;

    if (length < 1) {
        return NULL;
    }
    
    if (stop > self->length) {
        stop = self->length;
    }

    sliced_list = List_new(1, sizeof(void *));
    for (size_t i = start; i < stop; i++) {
        List_push(sliced_list, self->list[i], sizeof(void *));
    }

    return sliced_list;
}

void List_copy_inplace(ListT *self, ListT *dest, size_t size) {
    for (size_t i = 0; i < self->length; i++) {
        List_push(dest, List_get(self, i), size);
    }
}
