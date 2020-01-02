#include "hw4.h"

#define NUM_TRAIN_INPUT 60000
#define NUM_TEST_INPUT 10000
#define INPUT_ELEM_SIZE 784
#define MAX_LABEL 10
#define MAX_THREAD 100

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

typedef struct {
    const double* X;
    const double* Y;
    double* Z;
    size_t start_row;
    size_t end_row;
} matrix_mul_param;

static void matrix_mul(
        const double X[][INPUT_ELEM_SIZE],
        const double Y[INPUT_ELEM_SIZE][MAX_LABEL],
        double Z[][MAX_LABEL],
        size_t start_row,
        size_t end_row) {
    /* Z := X * Y */
    LOG("matrix mul %d ~ %d", start_row, end_row);

    assert(0 <= start_row && start_row <= end_row && end_row <= NUM_TRAIN_INPUT);

    FOR(i, start_row, end_row) {
        FOR(j, 0, MAX_LABEL) {
            Z[i][j] = 0;
            FOR(k, 0, INPUT_ELEM_SIZE) {
                Z[i][j] += X[i][k] * Y[k][j];
            }
        }
    }
}

static void* matrix_mul_thread(void* ptr) {
    matrix_mul_param* param = (matrix_mul_param*)ptr;
    matrix_mul(param->X, param->Y, param->Z, param->start_row, param->end_row);
    return NULL;
}

static void matrix_mul_transposed(
        const double X[][INPUT_ELEM_SIZE],
        const double Y[][MAX_LABEL],
        double Z[INPUT_ELEM_SIZE][MAX_LABEL],
        size_t num_row) {
    /* Z := X^T * Y */

    assert(num_row == NUM_TRAIN_INPUT);

    FOR(i, 0, INPUT_ELEM_SIZE) {
        FOR(j, 0, MAX_LABEL) {
            Z[i][j] = 0;
            FOR(k, 0, num_row) {
                Z[i][j] += X[k][i] * Y[k][j];
            }
        }
    }
}

static void softmax(const double vector[], double dest[], size_t dimension) {
    double mx = -INFINITY;
    double sum = 0;

    FOR(i, 0, dimension)
        if (mx < vector[i])
            mx = vector[i];

    FOR(i, 0, dimension)
        sum += exp(vector[i] - mx);

    FOR(i, 0, dimension)
        dest[i] = exp(vector[i] - mx) / sum;
}

pthread_t thread[MAX_THREAD];
matrix_mul_param thread_param[MAX_THREAD];

// training input
int num_thread;
int num_iteration = 100;
double learning_rate = 0.8;
double x_train[NUM_TRAIN_INPUT][INPUT_ELEM_SIZE]; // images for training
double y_train[NUM_TRAIN_INPUT][MAX_LABEL]; // labels for training

// testing input
double x_test[NUM_TEST_INPUT][INPUT_ELEM_SIZE]; // images for testing

// trained data
double weight[INPUT_ELEM_SIZE][MAX_LABEL];
double weight_grad[INPUT_ELEM_SIZE][MAX_LABEL];
double y_hat[NUM_TRAIN_INPUT][MAX_LABEL]; // predicted label

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

        static double tmp[NUM_TRAIN_INPUT][MAX_LABEL];

        size_t num_row_per_thread = NUM_TRAIN_INPUT / (num_thread - 1);
        size_t start_row = 0;
        FOR(i, 0, num_thread) {

//             matrix_mul(x_train, weight, tmp, start_row, start_row + num_row_a_round);
//

            thread_param[i].X = x_train;
            thread_param[i].Y = weight;
            thread_param[i].Z = tmp;
            thread_param[i].start_row = start_row;
            thread_param[i].end_row = start_row + num_row_per_thread;

            if (thread_param[i].end_row > NUM_TRAIN_INPUT)
                thread_param[i].end_row = NUM_TRAIN_INPUT;

            pthread_create(&thread[i], NULL, matrix_mul_thread, &thread_param[i]);

            start_row += num_row_per_thread;
        }
//         pthread_exit(NULL); // Note: main theard quits also ?
        FOR(i, 0, num_thread) {
            pthread_join(thread[i], NULL);
        }

        LOG("x_train * weight");
        PRINTARR(tmp, 100, 10);
        LOG("");

        FOR(i, 0, NUM_TRAIN_INPUT) {
            softmax(tmp[i], y_hat[i], MAX_LABEL);
        }

        LOG("y_hat");
        PRINTARR(y_hat, 100, 10);
        LOG("");

        FOR(i, 0, INPUT_ELEM_SIZE) {
            FOR(j, 0, MAX_LABEL) {
                weight[i][j] -= learning_rate * weight_grad[i][j];
            }
        }

        LOG("weight");
        PRINTARR(weight, 100, 10);
        LOG("");

        FOR(i, 0, NUM_TRAIN_INPUT) {
            FOR(j, 0, MAX_LABEL) {
                tmp[i][j] = y_hat[i][j] - y_train[i][j];
            }
        }

        matrix_mul_transposed(x_train, tmp, weight_grad, NUM_TRAIN_INPUT);

        LOG("weight_grad");
        PRINTARR(weight_grad, 100, 10);
        LOG("");
    }
}

static void test() {
    LOG("test");

    static double tmp[NUM_TEST_INPUT][MAX_LABEL];

    matrix_mul(x_test, weight, tmp, 0, NUM_TEST_INPUT);

    FILE* f = fopen("result.csv", "w");
    fprintf(f, "id,label\n");

    FOR(i, 0, NUM_TEST_INPUT) {
        int label = -1;
        double mx = 0;
        FOR(j, 0, MAX_LABEL) {
            if (mx < tmp[i][j]) {
                mx = tmp[i][j];
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
                "Usage: %s [X_train] [y_train] [X_test] [number of threads]\n",
                argv[0]);
        exit(123);
    }


    const uchar* x_train_file = open_large_readonly(argv[1]);
    const uchar* y_train_file = open_large_readonly(argv[2]);
    const uchar* x_test_file  = open_large_readonly(argv[3]);

    num_thread = strtol(argv[4], NULL, MAX_LABEL);
    if (errno != 0) goto usage;


    FOR(i, 0, NUM_TRAIN_INPUT) {
        FOR(j, 0, INPUT_ELEM_SIZE) {
            x_train[i][j] = x_train_file[i * INPUT_ELEM_SIZE + j];
        }
    }

    FOR(i, 0, NUM_TRAIN_INPUT) {
        assert(y_train_file[i] < MAX_LABEL);
        y_train[i][ y_train_file[i] ] = 1;
    }

    FOR(i, 0, NUM_TEST_INPUT) {
        FOR(j, 0, INPUT_ELEM_SIZE) {
            x_test[i][j] = x_test_file[i * INPUT_ELEM_SIZE + j];
        }
    }

    train();
    test();

    return 0;
}
