#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug.h"
#include "sftp_list.h"
#include "sftp_utils.h"

#define MAX_COLS 128

uint32_t
get_window_column_length(void) {
    struct winsize window_size;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    return window_size.ws_col;
}

uint32_t
u32_array_sum(const uint32_t *array, size_t length) {
    uint32_t sum = 0;

    for (size_t i = 0; i < length; i++) {
        sum += array[i];
    }

    return sum;
}

void
u32_array_copy(uint32_t *dest, size_t *len_dest, const uint32_t *src, size_t len_src) {
    for (size_t i = 0; i < len_src; i++) {
        dest[i] = src[i];
    }
    *len_dest = len_src;
}

size_t
char_list_max_len(ListT *self) {
    size_t len;
    size_t len_max = 0;

    for (size_t i = 0; i < self->length; i++) {
        len = strlen(List_get(self, i));

        if (len_max < len) {
            len_max = len;
        }
    }

    return len_max;
}

ListT *
char_list_slice(ListT *self, size_t start, size_t stop) {
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
        List_push(sliced_list, self->list[i], strlen(self->list[i]) + 1);
    }

    return sliced_list;
}

ListT *
char_list_equalized_slice(ListT *self, size_t len_rows, size_t len_cols) {
    ListT *rows = List_new(1, sizeof *rows);
    ListT *row, *slice;
    size_t start, stop;
    char *list_item;

    for (size_t i = 0; i < len_rows; i++) {
        row = List_new(1, sizeof(char));

        for (size_t j = 0; j < len_cols; j++) {
            start = len_rows * j;
            stop = len_rows * (j + 1);

            slice = char_list_slice(self, start, stop);
            list_item = List_get(slice, i);
            if (i < slice->length) {
                List_push(row, list_item, strlen(list_item) + 1);
            }
        }
        List_push(rows, row, (sizeof *row));
    }

    return rows;
}

void
char_list_format_columnwise(ListT *self, size_t width_screen, char *delimiter) {
    size_t len_cols = 2;
    size_t len_delimiter = strlen(delimiter);
    size_t len_rows, start, stop, len_max, len_buf_widths, len_col_widths = 0;
    uint32_t col_widths[MAX_COLS] = {0};
    uint32_t buf_widths[MAX_COLS] = {0};
    ListT *slice, *rows = NULL, *row = NULL;

    len_max = char_list_max_len(self);

    if ((len_max + len_delimiter) * self->length < width_screen) {
        for (size_t i = 0; i < self->length; i++) {
            printf("%s%s", ((char *)List_get(self, i)), delimiter);
        }
        putchar('\n');
        return;
    }

    for (;; len_cols++) {
        len_rows = CEIL(self->length, len_cols);
        len_buf_widths = 0;

        for (size_t i = 0; i < len_cols && (start = len_rows * i) <= self->length; i++) {
            stop = len_rows * (i + 1);

            slice = char_list_slice(self, start, stop);
            len_max = char_list_max_len(slice);
            buf_widths[len_buf_widths++] = len_max;
        }

        if (u32_array_sum(buf_widths, len_buf_widths) + len_delimiter * (len_cols - 1) >
            width_screen) {
            break;
        }

        u32_array_copy(col_widths, &len_col_widths, buf_widths, len_buf_widths);
        rows = char_list_equalized_slice(self, len_rows, len_cols);

        List_free(slice);
    }

    for (size_t i = 0; i < rows->length; i++) {
        row = List_get(rows, i);
        for (size_t j = 0; j < row->length; j++) {
            printf("%-*s%s", col_widths[j], (char *)List_get(row, j), delimiter);
        }
        putchar('\n');
    }

    if (row != NULL) {
        List_free(row);
    }
    if (rows != NULL) {
        List_free(rows);
    }
}

bool
check_show_hidden(char *path_str, size_t length, uint8_t flag) {
    return BIT_MATCH(flag, 0) ? 1 : !path_is_hidden(path_str, length);
}

bool
check_path_type(char *path_str, size_t length, bool is_dir, uint8_t flag) {
    uint8_t is_valid = check_show_hidden(path_str, length, flag);

    if (BIT_MATCH(flag, 1)) {
        return is_dir && is_valid;
    }

    return is_valid;
}
