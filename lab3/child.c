#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <semaphore.h>

#define BUFFER_SIZE 256
#define SHM_SIZE 1024

bool is_vowel(char c) {
    c = (c >= 'A' && c <= 'Z') ? c + 32 : c;
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

void remove_vowels(char *str) {
    int write_index = 0;
    for (int read_index = 0; str[read_index] != '\0'; read_index++) {
        if (!is_vowel(str[read_index])) {
            str[write_index++] = str[read_index];
        }
    }
    str[write_index] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        const char *error_msg = "Error: The file name, shared memory, or semaphore was not passed\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file == -1) {
        const char *error_msg = "Error: the file could not be opened for writing\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");
        return 1;
    }
    int shm_fd = shm_open(argv[2], O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    char *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    sem_t *sem = sem_open(argv[3], 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        sem_wait(sem);

        if (shm_ptr[0] != '\0') {
            strcpy(buffer, shm_ptr);
            if (strcmp(buffer, "exit") == 0) {
                sem_post(sem);
                break;
            }
            shm_ptr[0] = '\0';
            sem_post(sem);


            remove_vowels(buffer);
            if (write(file, buffer, strlen(buffer)) == -1 || write(file, "\n", 1) == -1) {
                const char *error_msg = "Error: failed to write to a file\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                close(file);
                return 1;
            }
        } else {
            sem_post(sem);
            usleep(100000);
        }
    }

    close(file);
    munmap(shm_ptr, SHM_SIZE);
    sem_close(sem);

    return 0;
}