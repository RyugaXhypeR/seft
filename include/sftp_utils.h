#ifndef SFTP_UTILS_H
#define SFTP_UTILS_H

#include <stdint.h>

#define BIT_MATCH(bits, pos) (bits & (1 << pos))
#define BIT_SET(bits, mask) (bits |= (1 << pos))

uint32_t get_window_column_length(void);

#endif /* ifndef SFTP_UTILS_H */
