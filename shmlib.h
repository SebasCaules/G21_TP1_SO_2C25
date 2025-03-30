#ifndef SHMLIB_H
#define SHMLIB_H

#include <sys/types.h>  // mode_t, off_t
#include <sys/mman.h>   // mmap, PROT_READ, etc.
#include <sys/stat.h>   // fstat, struct stat
#include <fcntl.h>      // O_CREAT, O_RDWR, etc.
#include <stdlib.h>     // exit, EXIT_FAILURE
#include <stdio.h>      // perror
#include <unistd.h>     // ftruncate, close


typedef struct {
    int fd;
    void *ptr;
    int size;
} SHMData;

/**
 * @brief Crea (o abre) una memoria compartida con nombre 'name' y tamaño 'size'.
 *
 * - Crea con permisos 0666.
 * - Usa ftruncate() para definir el tamaño.
 * - Devuelve un puntero mapeado con PROT_READ | PROT_WRITE, MAP_SHARED.
 * - Sale por exit(EXIT_FAILURE) si hay error.
 *
 * @param name   Nombre de la memoria compartida, ej: "/shm_test"
 * @param size   Tamaño en bytes de la memoria.
 * @param mode   Permisos de la memoria compartida.
 * @return       Puntero al mapeo en memoria.
 */
void *createSHM(char *name, size_t size, mode_t mode);

/**
 * @brief Borra una memoria compartida con nombre 'name'.
 *
 * - Hace shm_unlink().
 * - Sale por exit(EXIT_FAILURE) si hay error.
 *
 * @param name   Nombre de la memoria compartida, ej: "/shm_test"
 */
void eraseSHM(char *name);

/**
 * @brief Abre una memoria compartida existente con el flag 'oflag' (O_RDWR, O_RDONLY).
 *
 * - Hace shm_open con 'oflag'.
 * - Obtiene su tamaño con fstat().
 * - Mapea con PROT_READ (o PROT_READ|PROT_WRITE si querés).
 * - Sale por exit(EXIT_FAILURE) si hay error.
 *
 * @param name   Nombre de la memoria compartida, ej: "/shm_test"
 * @param oflag  Flags de apertura (O_RDWR, O_RDONLY, etc.)
 * @return       Puntero al mapeo en memoria.
 */
SHMData *openSHM(char* name, int oflag);

/**
 * @brief Libera una memoria compartida mapeada y cierra su file descriptor.
 * 
 * - Hace munmap() y close().
 * - Sale por exit(EXIT_FAILURE) si hay error.
 *
 * @param ptr     Puntero devuelto por mmap()
 * @param size    Tamaño del mapeo (por ejemplo obtenido con fstat)
 * @param fd      File descriptor devuelto por shm_open
 */
void closeSHM(SHMData *data);

#endif // SHMLIB_H