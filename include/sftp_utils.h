#ifndef SFTP_UTILS_H
#define SFTP_UTILS_H

#include <stdint.h>
#include "sftp_list.h"

#define BIT_MATCH(bit_mask, pos) ((bit_mask) & (1 << (pos)))
#define BIT_SET(bit_mask, pos) ((bit_mask) |= (1 << (pos)))
#define CEIL(dividend, divisor) ((dividend) / (divisor) + (dividend) % (divisor))

#define BUF_SIZE_FORMATTED_COLUMNWISE 16384
#define BUF_SIZE_LINE_LIMIT 1024

uint32_t get_window_column_length(void);
uint32_t u32_list_sum(ListT *self);
size_t char_list_max_len(ListT *self);
ListT *char_list_equalized_slice(ListT *self, size_t len_rows, size_t len_cols);
void char_list_format_columnwise(ListT *self, size_t width_screen, char *delimiter);
bool check_show_hidden(char *path_str, size_t length, uint8_t flag);
bool check_path_type(char *path_str, size_t length, bool is_dir, uint8_t flag);

#endif /* ifndef SFTP_UTILS_H */
