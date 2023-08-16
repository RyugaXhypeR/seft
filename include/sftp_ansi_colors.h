#ifndef SFTP_ANSI_COLORS_H
#define SFTP_ANSI_COLORS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define ANSI_COLOR_COMBINED_LEN 9

#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_UNDERLINE "\x1b[4m"
#define ANSI_BLINK "\x1b[5m"
#define ANSI_REVERSE "\x1b[7m"
#define ANSI_INVISIBLE "\x1b[8m"
#define ANSI_STRIKETHROUGH "\x1b[9m"
#define ANSI_BOLD_OFF "\x1b[21m"
#define ANSI_UNDERLINE_OFF "\x1b[24m"
#define ANSI_BLINK_OFF "\x1b[25m"
#define ANSI_REVERSE_OFF "\x1b[27m"
#define ANSI_INVISIBLE_OFF "\x1b[28m"
#define ANSI_STRIKETHROUGH_OFF "\x1b[29m"
#define ANSI_FG_BLACK "\x1b[30m"
#define ANSI_FG_RED "\x1b[31m"
#define ANSI_FG_GREEN "\x1b[32m"
#define ANSI_FG_YELLOW "\x1b[33m"
#define ANSI_FG_BLUE "\x1b[34m"
#define ANSI_FG_MAGENTA "\x1b[35m"
#define ANSI_FG_CYAN "\x1b[36m"
#define ANSI_FG_WHITE "\x1b[37m"

#endif /*!SFTP_ANSI_COLORS_H */
