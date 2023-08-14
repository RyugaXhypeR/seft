#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sftp_utils.h"

uint32_t
get_window_column_length(void) {
    struct winsize window_size;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    return window_size.ws_col;
}
