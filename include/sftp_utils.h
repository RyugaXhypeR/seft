#ifndef SFTP_UTILS_H
#define SFTP_UTILS_H

#include <stdint.h>
#include "sftp_list.h"

/** Macro to check if a bit is set in a bit mask */
#define BIT_MATCH(bit_mask, pos) ((bit_mask) & (1UL << (pos)))

/** Macro to set a bit in a bit mask */
#define BIT_SET(bit_mask, pos) ((bit_mask) |= (1UL << (pos)))

/** Macro to get the ceiling of a division */
#define CEIL(dividend, divisor) ((dividend) / (divisor) + (dividend) % (divisor))

/** Macro to check if ``__VA_ARGS__`` passed to a macro is empty */
#define VA_ARGS_IS_EMPTY(...) (sizeof((char[]){#__VA_ARGS__}) == 1)

uint32_t get_window_column_length(void);
uint32_t u32_list_sum(ListT *self);
size_t char_list_max_len(ListT *self);
ListT *char_list_equalized_slice(ListT *self, size_t len_rows, size_t len_cols);
void char_list_format_columnwise(ListT *self, size_t width_screen, char *delimiter);
bool check_show_hidden(char *path_str, size_t length, uint8_t flag);
bool check_path_type(char *path_str, size_t length, bool is_dir, uint8_t flag);

#endif /* ifndef SFTP_UTILS_H */
