#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

// guard for syscall error
#define G(expr) if ((expr) < 0) { perror(#expr); exit(221); }

#define FOR(i, start, end) for (size_t i = start; i < end; ++i)

#define PRINTARR(arr, r, c) \
    for (size_t i = 0; i < r; ++i) {\
        for (size_t j = 0; j < c; ++j) {\
            fprintf(stderr, "%lf ", arr[i][j]);\
        }\
        fprintf(stderr, "\n");\
    }

#ifdef DEBUG
# define LOG(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#else
# define LOG(...)
#endif

typedef unsigned char uchar;

