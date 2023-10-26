/** A very minimal logger to handle basic logging for the project */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#ifdef D
#define DBG_STATUS 1
#else
#define DBG_STATUS 0
#endif  // !D

#include <stdio.h>

#include "seft_ansi_colors.h"

enum DBG_LEVELS {
    DBG_LEVEL_DEBUG,
    DBG_LEVEL_INFO,
    DBG_LEVEL_CRITICAL,
};

static size_t allocated = 0;

/** Simple logger macro
 *
 * .. note:: Currently requires you to pass in variadic args to it
 * */
#define LOG(level, file, line, func, prompt, ...)                                        \
    do {                                                                                 \
        if (level == DBG_LEVEL_CRITICAL) {                                               \
            fprintf(stderr, "[%s]::" prompt "\n", dbg_level_to_str(level), __VA_ARGS__); \
        }                                                                                \
        else if (DBG_STATUS) {                                                                \
            fprintf(stderr, "[%s]:%s:%d:%s: " prompt "\n", dbg_level_to_str(level),      \
                    file, line, func, __VA_ARGS__);                                      \
        }                                                                                \
    } while (0)

#define DBG_ERR(prompt, ...) \
    LOG(DBG_LEVEL_CRITICAL, __FILE__, __LINE__, __func__, prompt, __VA_ARGS__)
#define DBG_INFO(prompt, ...) \
    LOG(DBG_LEVEL_INFO, __FILE__, __LINE__, __func__, prompt, __VA_ARGS__)
#define DBG_DEBUG(prompt, ...) \
    LOG(DBG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, prompt, __VA_ARGS__)

#define DBG_MALLOC(size) dbg_malloc(size, __FILE__, __LINE__, __func__)
#define DBG_CALLOC(num_bytes, type_size) \
    dbg_calloc(num_bytes, type_size, __FILE__, __LINE__, __func__)
#define DBG_REALLOC(ptr, new_size) \
    dbg_realloc(ptr, new_size, __FILE__, __LINE__, __func__)
#define DBG_SAFE_FREE(ptr) dbg_safe_free(ptr, __FILE__, __LINE__, __func__)

static inline char*
dbg_level_to_str(enum DBG_LEVELS level) {
    switch (level) {
        case DBG_LEVEL_DEBUG:
            return ANSI_FG_GREEN "DEBUG" ANSI_RESET;
        case DBG_LEVEL_INFO:
            return ANSI_FG_CYAN "INFO" ANSI_RESET;
        case DBG_LEVEL_CRITICAL:
            return ANSI_FG_RED "CRITICAL" ANSI_RESET;
    }

    return "";
}

/** Log the size and *malloc* memory */
static inline void*
dbg_malloc(size_t size, char const* file, int line, char const* func) {
    void* ptr = malloc(size);

    if (ptr == NULL) {
        DBG_ERR("Unable to allocate %zu bytes of memory", size);
        return NULL;
    }
    allocated += size;

    LOG(DBG_LEVEL_DEBUG, file, line, func,
        ANSI_FG_GREEN "Allocated: " ANSI_RESET ANSI_FG_BLUE "%zu" ANSI_RESET " bytes",
        size);

    return ptr;
}

/** Log the size and *calloc* memory */
static inline void*
dbg_calloc(size_t num_bytes, size_t type_size, char const* file, int line,
           char const* func) {
    void* ptr = calloc(num_bytes, type_size);

    if (ptr == NULL) {
        DBG_ERR("Unable to allocate %zu bytes of memory", num_bytes * type_size);
        return NULL;
    }
    allocated += num_bytes * type_size;

    LOG(DBG_LEVEL_DEBUG, file, line, func,
        ANSI_FG_GREEN "Allocated: " ANSI_RESET ANSI_FG_BLUE "%zu" ANSI_RESET " bytes",
        num_bytes * type_size);

    return ptr;
}

/** Log the size and *relloc* memory for the pointer */
static inline void*
dbg_realloc(void* ptr, size_t new_size, char const* file, int line, char const* func) {
    void* new_allocated = realloc(ptr, new_size);

    if (new_allocated == NULL) {
        DBG_ERR("Unable to reallocate memory for pointer %p", ptr);
        return NULL;
    }
    allocated += new_size;

    LOG(DBG_LEVEL_DEBUG, file, line, func,
        ANSI_FG_GREEN "Reallocated: " ANSI_RESET ANSI_FG_BLUE "%zu" ANSI_RESET " bytes",
        new_size);

    return new_allocated;
}

/** Free if pointer is not NULL and log the result */
static inline void
dbg_safe_free(void* ptr, char const* file, int line, char const* func) {
    if (ptr == NULL) {
        DBG_INFO("Pointer is null, not freeing %s", "");
        return;
    }

    free(ptr);
    LOG(DBG_LEVEL_DEBUG, file, line, func,
        ANSI_FG_GREEN "Freed: " ANSI_RESET ANSI_FG_BLUE "%p" ANSI_RESET, ptr);
}

#endif /* DEBUG_H */
