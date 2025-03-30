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
 * - Mapea con PROT_READ o PROT_READ | PROT_WRITE dependiendo del flag.
 * - Sale por exit(EXIT_FAILURE) si hay error.
 *
 * @param name   Nombre de la memoria compartida, ej: "/game_state"
 * @param oflag  Flags de apertura (O_RDONLY, O_RDWR, etc.)
 * @return       Puntero al mapeo en memoria (resultado de mmap)
 */
void* openSHM(const char* name, int oflag, size_t *sizeOutput);

/**
 * @brief Libera una memoria compartida mapeada y cierra su descriptor usando solo el nombre.
 *
 * - Hace fstat para conocer el tamaño.
 * - Ejecuta munmap() y close() internamente.
 * - No devuelve nada. Muestra errores con perror.
 *
 * @param name   Nombre de la memoria compartida, ej: "/game_state"
 * @param ptr    Puntero devuelto por openSHM (resultado de mmap)
 */
void closeSHM(void* ptr, size_t size);

#endif // SHMLIB_H