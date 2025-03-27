// fork + shm + sem
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>  /* For mode constants */
#include <fcntl.h>     /* For O_* constants */
#include <sys/wait.h>
#include <semaphore.h> // Corrected include

void *createSHM(char *name, size_t size) {
    int fd;
    //int shm_open(const char *name, int oflag, mode_t mode);

    fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    //int ftruncate(int fd, off_t length);
    if (-1 == ftruncate(fd, size)) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    
    // void *mmap(void addr[.length], size_t length, int prot, int flags, int fd, off_t offset);
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

int main(int argc, char* argv[]) {
    char *p = (char*) createSHM("/shm_test", 8); // 5 dígitos + '\n' + '\0'
    printf("Shared memory created at %p\n", p);

    for (int i = 0; i < 5; i++) {
        p[i] = '1' + i; // escribe '1', '2', ..., '5'
    }
    p[5] = '\n';
    p[6] = '\0'; // null terminator para seguridad

    wait(NULL); // Esperar al hijo

    // Proceso padre: simula el "cat"
    int fd = open("/dev/shm/shm_test", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    char buffer[8 + 1] = {0};
    read(fd, buffer, 8);
    close(fd);

    printf("Contenido en /dev/shm/shm_test: %s", buffer);

    pid_t pid;

    /*
    int sem_init(sem_t *sem, int pshared, unsigned int value);
    sem: El semaforo a inicializar
    pshared: 0 si es compartido entre hilos, 1 si es compartido entre procesos, valor inicial
    value: Valor inicial del semaforo
    */
    sem_t *print_needed = (sem_t*) createSHM("/print_needed", sizeof(sem_t));
    if(sem_init(print_needed, 1, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    sem_t *print_done = (sem_t*) createSHM("/print_done", sizeof(sem_t));
    if(sem_init(print_done, 1, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    switch (pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        case 0:
            for (int i = 0; i < 3; i++) {
                sem_wait(print_needed);
                printf("\t*p: %d\n", *p);
                sem_post(print_done);
            }
            exit(EXIT_SUCCESS);

        default:
            for (int i = 0; i < 3; i++) {
                *p = i;
                printf("*p: %d\n", i);
                sem_post(print_needed);
                sem_wait(print_done);
            }
            exit(EXIT_SUCCESS);
    }

    return 0;
}