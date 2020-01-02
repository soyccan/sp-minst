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

static uchar* read_large(const char* filename) {

    int fd;
    G(fd = open(filename, O_RDONLY));

    static uchar buf[47040000];
    G(read(fd, buf, getsize(fd)));

    return buf;
}

// thread
pthread_t thread[MAX_THREAD];
struct {
    size_t start_row;
    size_t end_row;
} thread_param[MAX_THREAD];

// training input
int num_thread;
int num_iteration = 3;
double learning_rate = 0.0004;
double x_train[NUM_TRAIN_INPUT][INPUT_ELEM_SIZE]; // images for training
double y_train[NUM_TRAIN_INPUT][MAX_LABEL]; // labels for training

// testing input
double x_test[NUM_TEST_INPUT][INPUT_ELEM_SIZE]; // images for testing

// trained data
double weight[INPUT_ELEM_SIZE][MAX_LABEL];
double weight_grad[INPUT_ELEM_SIZE][MAX_LABEL];
double y_hat[NUM_TRAIN_INPUT][MAX_LABEL]; // predicted label


static void* matrix_mul_thread(void* ptr) {
    size_t idx = ptr;
    size_t start_row = thread_param[idx].start_row;
    size_t end_row = thread_param[idx].end_row;

    LOG("matrix mul %d ~ %d", start_row, end_row);

    assert(start_row <= end_row && end_row <= NUM_TRAIN_INPUT);

    FOR(i, start_row, end_row) {
        FOR(j, 0, MAX_LABEL) {
            y_hat[i][j] = 0;
            FOR(k, 0, INPUT_ELEM_SIZE) {
                y_hat[i][j] += x_train[i][k] * weight[k][j];
            }
        }
    }
    return NULL;
}

static void train() {
    LOG("train");

    // optional
    FOR(i, 0, INPUT_ELEM_SIZE) {
        FOR(j, 0, MAX_LABEL) {
            weight[i][j] = 0.5;
        }
    }

    FOR(rnd, 0, num_iteration) {
        LOG("train round %d", rnd);


        // step (1)
        size_t num_row_per_thread = NUM_TRAIN_INPUT;
        if (num_thread > 1)
            num_row_per_thread /= (num_thread - 1);

        size_t start_row = 0;
        FOR(i, 0, num_thread) {
            thread_param[i].start_row = start_row;
            thread_param[i].end_row = start_row + num_row_per_thread;

            if (thread_param[i].end_row > NUM_TRAIN_INPUT)
                thread_param[i].end_row = NUM_TRAIN_INPUT;

            pthread_create(&thread[i], NULL, matrix_mul_thread, i);

            start_row += num_row_per_thread;
        }
//         pthread_exit(NULL); // Note: main theard quits also ?
        FOR(i, 0, num_thread) {
            pthread_join(thread[i], NULL);
        }

        LOG("x_train * weight");
        PRINTARR(y_hat, 100, 10);
        LOG("");

        // step (2)
        FOR(i, 0, NUM_TRAIN_INPUT) {
            double mx = -INFINITY;
            double sum = 0;

            FOR(j, 0, MAX_LABEL)
                if (mx < y_hat[i][j])
                    mx = y_hat[i][j];

            FOR(j, 0, MAX_LABEL)
                sum += exp(y_hat[i][j] - mx);

            FOR(j, 0, MAX_LABEL)
                y_hat[i][j] = exp(y_hat[i][j] - mx) / sum;
        }

        LOG("y_hat");
        PRINTARR(y_hat, 100, 10);
        LOG("");

        // step (3)
        FOR(i, 0, INPUT_ELEM_SIZE) {
            FOR(j, 0, MAX_LABEL) {
                weight[i][j] -= learning_rate * weight_grad[i][j];
            }
        }

        LOG("weight");
        PRINTARR(weight, 100, 10);
        LOG("");

        // step (4)
        FOR(i, 0, INPUT_ELEM_SIZE) {
            FOR(j, 0, MAX_LABEL) {
                weight_grad[i][j] = 0;
                FOR(k, 0, NUM_TRAIN_INPUT) {
                    weight_grad[i][j] += x_train[k][i] * (y_hat[k][j] - y_train[k][j]);
                }
            }
        }

        LOG("weight_grad");
        PRINTARR(weight_grad, 100, 10);
        LOG("");
    }
}

static void test() {
    LOG("test");

    // step(1)
    FOR(i, 0, NUM_TEST_INPUT) {
        FOR(j, 0, MAX_LABEL) {
            y_hat[i][j] = 0;
            FOR(k, 0, INPUT_ELEM_SIZE) {
                y_hat[i][j] += x_test[i][k] * weight[k][j];
            }
        }
    }

    FILE* f = fopen("result.csv", "w");
    fprintf(f, "id,label\n");

    FOR(i, 0, NUM_TEST_INPUT) {
        int label = -1;
        double mx = 0;
        FOR(j, 0, MAX_LABEL) {
            if (mx < y_hat[i][j]) {
                mx = y_hat[i][j];
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

    uchar* buf;

    buf = read_large(argv[1]);
    FOR(i, 0, NUM_TRAIN_INPUT) {
        FOR(j, 0, INPUT_ELEM_SIZE) {
            x_train[i][j] = buf[i * INPUT_ELEM_SIZE + j];
        }
    }

    buf = read_large(argv[2]);
    FOR(i, 0, NUM_TRAIN_INPUT) {
        assert(buf[i] < MAX_LABEL);
        y_train[i][ buf[i] ] = 1;
    }

    buf = read_large(argv[3]);
    FOR(i, 0, NUM_TEST_INPUT) {
        FOR(j, 0, INPUT_ELEM_SIZE) {
            x_test[i][j] = buf[i * INPUT_ELEM_SIZE + j];
        }
    }

    train();
    test();

    return 0;
}
