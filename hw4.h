#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


#define FOR(i, start, end) for (size_t i = start; i < end; ++i)


#ifdef DEBUG
# define LOG(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
# define LOGN(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

// guard for syscall error
# define G(expr) if ((expr) < 0) { perror(#expr); exit(221); }

# define PRINTARR(arr, r, c) \
    for (size_t i = 0; i < r; ++i) {\
        for (size_t j = 0; j < c; ++j) {\
            fprintf(stderr, "%lf ", (double)arr[i][j]);\
        }\
        fprintf(stderr, "\n");\
    }

# define PRINTARR_T(arr, r, c) \
    for (size_t i = 0; i < r; ++i) {\
        for (size_t j = 0; j < c; ++j) {\
            fprintf(stderr, "%lf ", (double)arr[j][i]);\
        }\
        fprintf(stderr, "\n");\
    }

#else
# define LOG(...)
# define LOGN(...)
# define G(expr) (expr)
# define PRINTARR(...)
# define PRINTARR_T(...)
#endif

typedef unsigned char uchar;

