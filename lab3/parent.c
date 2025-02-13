#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define BUFFER_SIZE 256
#define SHM_SIZE 1024

int main() {
    char filename1[BUFFER_SIZE];
    char filename2[BUFFER_SIZE];
    ssize_t bytes_read;
    const char *prompt1 = "Enter the file name for child1: ";
    write(STDOUT_FILENO, prompt1, strlen(prompt1));
    bytes_read = read(STDIN_FILENO, filename1, BUFFER_SIZE - 1);
    filename1[bytes_read - 1] = '\0';
    const char *prompt2 = "Enter the file name for child2: ";
    write(STDOUT_FILENO, prompt2, strlen(prompt2));
    bytes_read = read(STDIN_FILENO, filename2, BUFFER_SIZE - 1);
    filename2[bytes_read - 1] = '\0';

    int file_check1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_check1 == -1) {
        const char *error_msg = "Error: could not open the file for child1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");
        return 1;
    }
    close(file_check1);

    int file_check2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_check2 == -1) {
        const char *error_msg = "Error: could not open the file for child2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");
        return 1;
    }
    close(file_check2);

    int shm_fd1 = shm_open("/shm1", O_CREAT | O_RDWR, 0666);
    int shm_fd2 = shm_open("/shm2", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd1, SHM_SIZE);
    ftruncate(shm_fd2, SHM_SIZE);

    char *shm_ptr1 = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
    char *shm_ptr2 = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);

    sem_t *sem1 = sem_open("/sem1", O_CREAT, 0666, 1);
    sem_t *sem2 = sem_open("/sem2", O_CREAT, 0666, 1);
    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        const char *error_msg = "Error when creating a child process 1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    } else if (pid1 == 0) {
        execl("./child.out", "./child.out", filename1, "/shm1", "/sem1", NULL);
        const char *error_msg = "Error when starting child1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        const char *error_msg = "Error when creating a child process 2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    } else if (pid2 == 0) {
        execl("./child.out", "./child.out", filename2, "/shm2", "/sem2", NULL);
        const char *error_msg = "Error when starting child2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(1);
    }

    char input[BUFFER_SIZE];
    while (1) {
        bytes_read = read(STDIN_FILENO, input, sizeof(input) - 1);
        if (bytes_read <= 0) {
            const char *error_msg = "Error while reading the string\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            break;
        }
        if (bytes_read > 0 && input[bytes_read - 1] == '\n') {
            input[bytes_read - 1] = '\0';
        }
        if (strcmp(input, "exit") == 0) {
            sem_wait(sem1);
            strcpy(shm_ptr1, input);
            sem_post(sem1);
            sem_wait(sem2);
            strcpy(shm_ptr2, input);
            sem_post(sem2);
            break;
        }

        int randomInt = rand() % 100;
        if (randomInt < 80) {
            sem_wait(sem1);
            strcpy(shm_ptr1, input);
            sem_post(sem1);
        } else {
            sem_wait(sem2);
            strcpy(shm_ptr2, input);
            sem_post(sem2);
        }
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    munmap(shm_ptr1, SHM_SIZE);
    munmap(shm_ptr2, SHM_SIZE);
    shm_unlink("/shm1");
    shm_unlink("/shm2");
    sem_close(sem1);
    sem_close(sem2);
    sem_unlink("/sem1");
    sem_unlink("/sem2");

    return 0;
}