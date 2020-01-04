#include "hw4.h"

#define NUM_TRAIN_INPUT 60000
#define NUM_TEST_INPUT 10000
#define INPUT_ELEM_SIZE 784
#define MAX_LABEL 10
#define MAX_THREAD 10000

static off_t getsize(int fd) {
    struct stat s;
    G(fstat(fd, &s));
    return s.st_size;
}

static const uchar* open_large_readonly(const char* filename) {
    uchar* pa;
    int fd;

    G(fd = open(filename, O_RDONLY));

    pa = mmap(NULL, getsize(fd), PROT_READ, MAP_SHARED, fd, 0);
    if (pa == MAP_FAILED)
        G(-1);

    return pa;
}

// thread
//
pthread_t thread[MAX_THREAD];

// training input
//
// Note: some matrix is stored transposed for cache-friendly
// (especially in step (4))
int num_thread;
int num_iteration = 20;
double learning_rate = 0.01;
int learning_rate_grad = 2; // learning rate will be divided
                            // by this each iteration
double x_train[NUM_TRAIN_INPUT][INPUT_ELEM_SIZE]; // images for training
double x_train_t[INPUT_ELEM_SIZE][NUM_TRAIN_INPUT]; // transposed
double y_train_t[MAX_LABEL][NUM_TRAIN_INPUT]; // labels for training

// testing input
//
double x_test[NUM_TEST_INPUT][INPUT_ELEM_SIZE]; // images for testing

// temporary data
//
double weight_grad_t[MAX_LABEL][INPUT_ELEM_SIZE];

// training output
//
double weight_t[MAX_LABEL][INPUT_ELEM_SIZE];
double y_hat_t[MAX_LABEL][NUM_TRAIN_INPUT]; // predicted label


static void* step1_thread(void* ptr) {
    // step(1)
    uint32_t start_row = ((uint64_t)ptr >> 32) & 0xffffffff;
    uint32_t end_row = (uint64_t)ptr & 0xffffffff;

    LOG("matrix mul %d ~ %d", start_row, end_row);

    assert(start_row <= end_row && end_row <= NUM_TRAIN_INPUT);

    FOR(i, start_row, end_row) {
        FOR(j, 0, MAX_LABEL) {
            y_hat_t[j][i] = 0;
            FOR(k, 0, INPUT_ELEM_SIZE) {
                y_hat_t[j][i] += x_train[i][k] * weight_t[j][k];
            }
        }
    }
    return NULL;
}

static void step1() {
    // step (1)
    size_t num_row_per_thread = NUM_TRAIN_INPUT;
    if (num_thread > 1)
        num_row_per_thread /= (num_thread - 1);

    uint32_t start_row = 0;
    FOR(i, 0, num_thread) {
        uint32_t end_row = start_row + num_row_per_thread;

        if (end_row > NUM_TRAIN_INPUT)
            end_row = NUM_TRAIN_INPUT;

        pthread_create(&thread[i], NULL, step1_thread,
                (uint64_t)start_row << 32 | end_row);

        start_row += num_row_per_thread;
    }
//         pthread_exit(NULL); // Note: main theard quits also ?
    FOR(i, 0, num_thread) {
        pthread_join(thread[i], NULL);
    }
}

static void step2() {
    // step (2)
    // softmax
    FOR(i, 0, NUM_TRAIN_INPUT) {
        double c = 0;
        double sum = 0;

        // average
        FOR(j, 0, MAX_LABEL)
            c += y_hat_t[j][i];
        c /= MAX_LABEL;

        FOR(j, 0, MAX_LABEL) {
            // debug only
            assert(-709 <= y_hat_t[j][i] - c && y_hat_t[j][i] - c <= 709);

            if (y_hat_t[j][i] - c < -709)
                // in case of underflow
                y_hat_t[j][i] = DBL_MIN;

            else if (y_hat_t[j][i] - c > 709)
                // in case of overflow
                y_hat_t[j][i] = 1e306;

            else
                y_hat_t[j][i] = exp(y_hat_t[j][i] - c);

            sum += y_hat_t[j][i];
        }

        FOR(j, 0, MAX_LABEL) {
            y_hat_t[j][i] /= sum;
            assert(y_hat_t[j][i] > -1e-10);
        }
    }
}

static void step3() {
    // step (3)
    FOR(j, 0, MAX_LABEL) {
        FOR(i, 0, INPUT_ELEM_SIZE) {
            weight_t[j][i] -= learning_rate * weight_grad_t[j][i];
        }
    }
}

// static void* step4_thread(void* ptr) {
//     // step (4)
//     size_t idx = ptr;
//     size_t start_col = thread_param[idx].start;
//     size_t end_col = thread_param[idx].end;

//     LOG("matrix mul transposed %d ~ %d", start_col, end_col);

//     assert(start_col <= end_col && end_col <= NUM_TRAIN_INPUT);

//     FOR(i, 0, INPUT_ELEM_SIZE) {
//         FOR(j, 0, MAX_LABEL) {
//             FOR(k, start_col, end_col) {
//                 weight_grad[i][j] += x_train[k][i] * (y_hat[k][j] - y_train[k][j]);
//             }
//         }
//     }
//     return NULL;
// }

static void step4() {
    // step (4)
    FOR(i, 0, INPUT_ELEM_SIZE) {
        FOR(j, 0, MAX_LABEL) {
            weight_grad_t[j][i] = 0;
            FOR(k, 0, NUM_TRAIN_INPUT) {
                weight_grad_t[j][i] += x_train_t[i][k] * (y_hat_t[j][k] - y_train_t[j][k]);
            }
        }
    }

    // multi-thread code
    // Note: deleted due to race condition
//     FOR(i, 0, INPUT_ELEM_SIZE) {
//         FOR(j, 0, MAX_LABEL) {
//             weight_grad[i][j] = 0;
//         }
//     }

//     size_t num_col_per_thread = NUM_TRAIN_INPUT;
//     if (num_thread > 1)
//         num_col_per_thread /= (num_thread - 1);

//     size_t start_col = 0;
//     FOR(i, 0, num_thread) {
//         thread_param[i].start = start_col;
//         thread_param[i].end = start_col + num_col_per_thread;

//         if (thread_param[i].end > NUM_TRAIN_INPUT)
//             thread_param[i].end = NUM_TRAIN_INPUT;

//         pthread_create(&thread[i], NULL, step4_thread, i);

//         start_col += num_col_per_thread;
//     }
//     // pthread_exit(NULL); // Note: main theard quits also ?
//     FOR(i, 0, num_thread) {
//         pthread_join(thread[i], NULL);
//     }
}

static void train() {
    LOG("train");

    FOR(rnd, 0, num_iteration) {
        LOG("train round %d", rnd);

        step1();

        LOG("x_train * weight");
        PRINTARR(y_hat_t, 100, 10);
        LOG("");

        step2();

        LOG("y_hat");
        PRINTARR(y_hat_t, 100, 10);
        LOG("");

        step3();

        LOG("weight");
        PRINTARR(weight_t, 100, 10);
        LOG("");

        step4();

        LOG("weight_grad");
        PRINTARR(weight_grad_t, 100, 10);
        LOG("");

        learning_rate /= learning_rate_grad;
    }
}

static void test() {
    LOG("test");

    // step(1)
    FOR(i, 0, NUM_TEST_INPUT) {
        FOR(j, 0, MAX_LABEL) {
            y_hat_t[j][i] = 0;
            FOR(k, 0, INPUT_ELEM_SIZE) {
                y_hat_t[j][i] += x_test[i][k] * weight_t[j][k];
            }
        }
    }
    LOG("test y_hat");
    PRINTARR(y_hat_t, 100, MAX_LABEL);

    FILE* f = fopen("result.csv", "w");
    fprintf(f, "id,label\n");

    FOR(i, 0, NUM_TEST_INPUT) {
        int label = -1;
        double mx = 0;
        FOR(j, 0, MAX_LABEL) {
            if (mx < y_hat_t[j][i]) {
                mx = y_hat_t[j][i];
                label = j;
            }
        }

        fprintf(f, "%d,%d\n", (int)i, label);
    }

    fclose(f);
    f = NULL;
}

int main(int argc, const char** argv) {
    if (argc < 5) {
usage:
        fprintf(stderr,
                "Usage: %s [X_train] [y_train] [X_test] [number of threads < 10000]\n",
                argv[0]);
        exit(123);
    }

    num_thread = strtol(argv[4], NULL, MAX_LABEL);
    if (errno != 0 || num_thread > MAX_THREAD)
        goto usage;


    const uchar* buf;

    buf = open_large_readonly(argv[1]);
    FOR(i, 0, NUM_TRAIN_INPUT) {
        FOR(j, 0, INPUT_ELEM_SIZE) {
            // Note: reduce data range: [0,255] -> [0,1]
            x_train[i][j] = x_train_t[j][i] = buf[i * INPUT_ELEM_SIZE + j] * 0.001;
        }
    }

    buf = open_large_readonly(argv[2]);
    FOR(i, 0, NUM_TRAIN_INPUT) {
        assert(buf[i] < MAX_LABEL);
        y_train_t[ buf[i] ][i] = 1;
    }

    buf = open_large_readonly(argv[3]);
    FOR(i, 0, NUM_TEST_INPUT) {
        FOR(j, 0, INPUT_ELEM_SIZE) {
            x_test[i][j] = buf[i * INPUT_ELEM_SIZE + j] * 0.001;
        }
    }

    train();
    test();

    return 0;
}
