#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define USAGE() { \
    fprintf(stderr, "Usage: %s [X_train] [y_train] [X_test] [number of threads]\n", argv[0]); exit(123); }

// guard for syscall error
#define G(expr) if ((expr) < 0) { perror(#expr); exit(221); }

#ifdef DEBUG
# define LOG(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#else
# define LOG(...)
# define NDEBUG 1
#endif

typedef unsigned char uchar;

typedef struct {
    uchar** X;
    uchar** Y;
    uchar** Z;
    size_t start_row;
    size_t end_row;
} matrix_mul_param;

