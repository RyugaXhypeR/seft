#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "sftp_client.h"
#include "sftp_list.h"
#include "sftp_path.h"
#include "sftp_utils.h"
#include "sftp_ansi_colors.h"

#define MAX_COLS 128

/** Helper function to get the length of the terminal window. */
uint32_t
get_window_column_length(void) {
    struct winsize window_size;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    return window_size.ws_col;
}

/** Find the sum of a ``u32`` list.
 *
 * .. note:: Currently, resultant sum is adjusted to match the length
 *    ansi-color-formatted string, its not the absolute length of sum.
 *    Won't work for lists which store non-ansi-color-formatted strings.
 */
uint32_t
u32_array_sum(const uint32_t *array, size_t length) {
    uint32_t sum = 0;

    for (size_t i = 0; i < length; i++) {
        sum += array[i];
    }

    return sum - ANSI_COLOR_COMBINED_LEN * (length + 1);
}

/** Copy a ``u32`` list.
 *
 * .. note:: This will also change ``len_dest`` to actual length of
 *    ``dest`` after copying.
 */
void
u32_array_copy(uint32_t *dest, size_t *len_dest, const uint32_t *src, size_t len_src) {
    for (size_t i = 0; i < len_src; i++) {
        dest[i] = src[i];
    }
    *len_dest = len_src;
}

/** Get the length of the longest string in the list */
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

/** Get the sum of the lengths of all strings in the list
 *
 * .. note:: Currently, resultant sum is adjusted to match the length
 *    ansi-color-formatted string, its not the absolute length of sum.
 *    Won't work for lists which store non-ansi-color-formatted strings.
 * */
size_t
char_list_sum_len(ListT *self) {
    size_t len = 0;

    for (size_t i = 0; i < self->length; i++) {
        len += strlen(List_get(self, i));
    }

    return len - ANSI_COLOR_COMBINED_LEN * (self->length + 1);
}

/** Get a slice of a ``ListT`` of ``char *``
 *
 * :param self: The list to slice
 * :param start: [INCLUSIVE] The starting index of the slice
 * :param stop: [EXCLUSIVE] The ending index of the slice
 */
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

    sliced_list = List_new(1, sizeof(char *));
    for (size_t i = start; i < stop; i++) {
        List_push(sliced_list, self->list[i], strlen(self->list[i]) + 1);
    }

    return sliced_list;
}

/** Get a slice of a ``ListT`` of ``char *`` with equalized lengths
 *  For example::
 *
 *      A list of strings: ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j]
 *      Equalized slice of length 3: [["a", "d", "g"], ["b", "e", "h"], ["c", "f"], ["i",
 * "j"]] Instead of: [["a", "b", "c"], ["d", "e", "f"], ["g", "h", "i"], ["j"]]
 *
 * :param self: The list to slice
 * :param len_rows: The number of rows in the slice
 * :param len_cols: The number of columns in the slice
 *
 * .. note:: The resultant slice will be a ``ListT`` of ``ListT`` of ``char *``
 *    with ``len_rows`` rows and ``len_cols`` columns.
 *
 */
ListT *
char_list_equalized_slice(ListT *self, size_t len_rows, size_t len_cols) {
    ListT *rows = List_new(1, sizeof *rows);
    ListT *row, *slice;
    size_t start, stop;
    char *list_item;

    for (size_t i = 0; i < len_rows; i++) {
        row = List_new(1, sizeof *list_item);

        for (size_t j = 0; j < len_cols; j++) {
            start = len_rows * j;
            stop = len_rows * (j + 1);

            slice = char_list_slice(self, start, stop);
            list_item = List_get(slice, i);
            if (list_item != NULL) {
                List_push(row, list_item, strlen(list_item) + 1);
            }
        }
        List_push(rows, row, (sizeof *row));
    }

    return rows;
}

/** Format a ``ListT`` of ``char *`` columnwise
 *
 * :param self: The list to format
 * :param width_screen: The width of the screen
 * :param delimiter: The delimiter to use between columns
 *
 * .. note:: This function will print the formatted list to stdout.
 */
void
char_list_format_columnwise(ListT *self, size_t width_screen, char *delimiter) {
    size_t len_cols = 2;
    size_t len_delimiter = strlen(delimiter);
    size_t len_rows, start, stop, len_max, len_buf_widths, len_col_widths = 0;
    uint32_t col_widths[MAX_COLS] = {0};
    uint32_t buf_widths[MAX_COLS] = {0};
    ListT *slice, *rows = NULL, *row = NULL;

    /** If the sum of the lengths of all strings in the list is less than
     *  the width of the screen, then print the list as is.
     */
    if (char_list_sum_len(self) + len_delimiter * self->length < width_screen) {
        for (size_t i = 0; i < self->length; i++) {
            printf("%s%s", ((char *)List_get(self, i)), delimiter);
        }
        putchar('\n');
        return;
    }

    /**
     * This algorithm is based on::
     *
     *      https://github.com/changyuheng/columnify.py/blob/main/columnify/columnify.py
     */
    for (;; len_cols++) {
        len_rows = CEIL(self->length, len_cols);
        len_buf_widths = 0;

        for (size_t i = 0; i < len_cols && (start = len_rows * i) < self->length; i++) {
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
        for (size_t j = 0; j < row->length - 1; j++) {
            printf("%-*s%s", col_widths[j], (char *)List_get(row, j), delimiter);
        }
        /* Print the last column without the delimiter */
        printf("%-*s\n", col_widths[row->length - 1],
               (char *)List_get(row, row->length - 1));
        List_free(row);
    }

    if (rows != NULL) {
        List_free(rows);
    }
}

/** Helper function to check if flag is set to ``show all`` and if the path satisfies the
 * flag */
bool
check_show_hidden(char *path_str, size_t length, uint8_t flag) {
    return BIT_MATCH(flag, FLAG_LIST_BIT_POS_ALL) ? 1 : !path_is_hidden(path_str, length);
}

/** Helper function to check if the ``path_str`` satisfies the flag. */
bool
check_path_type(char *path_str, size_t length, bool is_dir, uint8_t flag) {
    uint8_t is_valid = check_show_hidden(path_str, length, flag);

    /* If flag is set to ``show directories only`` */
    if (BIT_MATCH(flag, FLAG_LIST_BIT_POS_DIR_ONLY)) {
        return is_dir && is_valid;
    } else if (BIT_MATCH(flag, FLAG_LIST_BIT_POS_FILE_ONLY)) {
        return !is_dir && is_valid;
    }

    return is_valid;
}
