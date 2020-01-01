#include "hw4.h"

static off_t getsize(int fd) {
    struct stat s;
    G(fstat(fd, &s));
    return s.st_size;
}

static const uchar* open_large_readonly(const char* filename) {
    uchar* pa;
    int fd;

    G(fd = open(filename, O_RDONLY));
    G(pa = mmap(NULL, getsize(fd), PROT_READ, MAP_SHARED, fd, 0));

    return pa;
}

static void matrix_mul(
        const uchar X[][784],
        const uchar Y[784][10],
        uchar Z[][10],
        size_t start_row,
        size_t end_row) {
    /* Z := X * Y */

    assert(0 <= start_row && start_row <= end_row && end_row <= 60000);

    for (size_t i = start_row; i < end_row; ++i) {
        for (size_t j = 0; j < 10; ++j) {
            Z[i][j] = 0;
            for (size_t k = 0; k < 784; ++k) {
                Z[i][j] += X[i][k] * Y[k][j];
            }
        }
    }
}

static void matrix_mul_transposed(
        const uchar X[][784],
        const uchar Y[][10],
        uchar Z[784][10],
        size_t num_row) {
    /* Z := X^T * Y */

    assert(num_row == 600000);

    for (size_t i = 0; i < 784; ++i) {
        for (size_t j = 0; j < 10; ++j) {
            Z[i][j] = 0;
            for (size_t k = 0; k < num_row; ++k) {
                Z[i][j] = X[k][i] * Y[k][j];
            }
        }
    }
}

static void softmax(const uchar vector[], uchar dest[], size_t dimension) {
    double sum = 0;

    for (size_t i = 0; i < dimension; ++i)
        sum += exp(vector[i]);

    for (size_t i = 0; i < dimension; ++i)
        dest[i] = exp(vector[i]) / sum;
}

static void* matrix_mul_thread(void* ptr) {
    matrix_mul_param* param = (matrix_mul_param*)ptr;
    matrix_mul(param->X, param->Y, param->Z, param->start_row, param->end_row);
    return NULL;
}

const int MAX_THREAD = 100;
pthread_t threads[100];

// input
int num_thread;
int num_iteration = 100;
double learning_rate = 0.8;
uchar x_train[60000][784]; // images for training
uchar y_train[60000][10]; // labels for training

// testing input
uchar x_test[10000][784]; // images for testing

// output
uchar weight[784][10];
uchar weight_grad[784][10];
uchar y_hat[60000][10];

void train() {
    for (int rnd = 0; rnd < num_iteration; ++rnd) {
        LOG("rnd=%d");

        static uchar tmp[60000][10];

        size_t num_row_a_round = 60000 / num_thread;
        for (size_t start_row = 0; start_row < 60000; start_row += num_row_a_round) {
            matrix_mul(x_train, weight, tmp, start_row, start_row + num_row_a_round);
        }

        for (int j = 0; j < 60000; ++j) {
            softmax(tmp[j], y_hat[j], 10);
        }

        for (int i = 0; i < 784; ++i) {
            for (int j = 0; j < 10; ++j) {
                weight[i][j] -= learning_rate * weight_grad[i][j];
            }
        }

        for (int i = 0; i < 60000; ++i) {
            for (int j = 0; j < 10; ++j) {
                tmp[i][j] = y_hat[i][j] - y_train[i][j];
            }
        }
        matrix_mul_transposed(x_train, tmp, weight_grad, 60000);
    }
}

void test() {
    static uchar tmp[10000][10];

    matrix_mul(x_test, weight, tmp, 0, 10000);

    FILE* f = fopen("result.csv", "w");
    fprintf(f, "id,label\n");

    for (int i = 0; i < 10000; ++i) {
        int label = -1;
        for (int j = 0; j < 10; ++j)
            if (tmp[i][j])
                label = j;

        fprintf(f, "%d,%d\n", i, label);
    }

    fclose(f);
    f = NULL;
}

int main(int argc, const char** argv) {
    if (argc < 5)
        USAGE();

    uchar* x_train_file = open_large_readonly(argv[1]);
    uchar* y_train_file = open_large_readonly(argv[2]);
    uchar* x_test_file  = open_large_readonly(argv[3]);

    num_thread = strtol(argv[4], NULL, 10);
    if (errno != 0) USAGE();

    memcpy(x_train, x_train_file, sizeof x_train);
    memcpy(y_train, y_train_file, sizeof y_train);
    memcpy(x_test , x_test_file , sizeof x_test);

    train();
    test();

    return 0;
}
