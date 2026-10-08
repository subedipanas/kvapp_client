#ifndef HELPERS_H
#define HELPERS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

[[noreturn]] static inline void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static inline uint32_t th_safe_size_t_to_u32(size_t val) {
    if (val > UINT32_MAX) {
        fprintf(stderr, "FATAL: size_t value (%zu) exceeds uint32_t limits.\n", val);
        abort();
    }

    return (uint32_t) val;
}

#endif
