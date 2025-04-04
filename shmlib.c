// fork + shm + sem
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>  /* For mode constants */
#include <fcntl.h>     /* For O_* constants */
#include <sys/wait.h>
#include <semaphore.h> // Corrected include
#include <unistd.h>    // Added for ftruncate

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

void eraseSHM(char *name) {
    if (shm_unlink(name) == -1) {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }
}


void* openSHM(const char* name, int oflag, size_t *sizeOutput) {
    int fd = shm_open(name, oflag, 0);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    *sizeOutput = st.st_size;

    int prot = (oflag & O_RDWR) ? (PROT_READ | PROT_WRITE) : PROT_READ;

    void* ptr = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    // Guardar temporalmente el fd asociado al nombre para cerrar después (usando shm_unlink si querés)
    // Alternativamente podrías usar un hashmap interno si necesitás reusar más adelante
    close(fd);  // opcional: cerrar acá, ya que el mapping ya está hecho
    return ptr;
}

void closeSHM(void* ptr, size_t size) {
    if (munmap(ptr, size) == -1) {
        perror("munmap");
    }
}