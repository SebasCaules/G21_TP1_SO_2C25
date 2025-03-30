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

void *createSHM(char *name, size_t size, mode_t mode) {
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, mode);
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