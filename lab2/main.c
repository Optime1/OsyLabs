#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


int *array;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    int left;
    int right;
} ThreadData;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int partition(int left, int right) {
    int pivot = array[right];
    int i = left - 1;

    for (int j = left; j <= right - 1; j++) {
        if (array[j] < pivot) {
            i++;
            swap(&array[i], &array[j]);
        }
    }
    swap(&array[i + 1], &array[right]);
    return i + 1;
}

void *quicksort(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int left = data->left;
    int right = data->right;

    if (left < right) {
        int pi = partition(left, right);

        ThreadData left_data = {left, pi - 1};
        ThreadData right_data = {pi + 1, right};

        pthread_t left_thread, right_thread;

        pthread_mutex_lock(&mutex);
        pthread_create(&left_thread, NULL, quicksort, &left_data);
        pthread_create(&right_thread, NULL, quicksort, &right_data);
        pthread_mutex_unlock(&mutex);

        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        const char msg[] = "Usage: %s <array_size> <max_threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_SUCCESS);
    }
    int size;
    size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    if (size <= 0 || num_threads <= 0) {
        const char msg[] = "Error: Array size and max threads must be positive integers.\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_SUCCESS);
    }
    array = (int*)malloc(size * sizeof(int));
    if (array == NULL) {
        const char msg[] = "Error: Cannot allocate memory.\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_SUCCESS);
    }

    clock_t start_creation, end_creation;
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 1000;
    }

    ThreadData data = {0, size - 1};
    pthread_t main_thread;

    start_creation = clock();
    pthread_create(&main_thread, NULL, quicksort, &data);
    pthread_join(main_thread, NULL);
    end_creation = clock();

    printf("Full time: %f seconds\n", (double)(end_creation - start_creation) / CLOCKS_PER_SEC);

    free(array);

    return 0;
}