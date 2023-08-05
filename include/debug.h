/** A very minimal logger to handle basic logging for the project */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef D
#define DBG_STATUS 1
#else
#define DBG_STATUS 0
#endif  // !D

#include <stdio.h>

enum DBG_LEVELS {
    DBG_LEVEL_DEBUG,
    DBG_LEVEL_INFO,
    DBG_LEVEL_CRITICAL,
};

#define LOG(level, prompt, ...)                                                       \
    do {                                                                              \
        if (DBG_STATUS) {                                                             \
            fprintf(stderr, "[%s]:%s:%d:%s: " prompt "\n", __dbg_level_to_str(level), \
                    __FILE__, __LINE__, __func__, __VA_ARGS__);                       \
        }                                                                             \
        if (level == DBG_LEVEL_CRITICAL) {                                            \
            fprintf(stderr, prompt "\n", __VA_ARGS__);                                \
        }                                                                             \
    } while (0)

#define DBG_ERR(prompt, ...) LOG(DBG_LEVEL_CRITICAL, prompt, __VA_ARGS__)
#define DBG_INFO(prompt, ...) LOG(DBG_LEVEL_INFO, prompt, __VA_ARGS__)
#define DBG_DEBUG(prompt, ...) LOG(DBG_LEVEL_DEBUG, prompt, __VA_ARGS__)

inline char*
__dbg_level_to_str(enum DBG_LEVELS level) {
    switch (level) {
        case DBG_LEVEL_DEBUG:
            return "DEBUG";
        case DBG_LEVEL_INFO:
            return "INFO";
        case DBG_LEVEL_CRITICAL:
            return "CRITICAL";
    }
}

#endif /* DEBUG_H */
