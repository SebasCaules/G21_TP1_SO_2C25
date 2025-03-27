#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>


typedef struct {
    char playerName[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int requestedInvalidMovements; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int requestedValidMovements; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y; // Coordenadas x e y en el tablero
    pid_t pid; // Identificador de proceso
    bool isBlocked; // Indica si el jugador está bloqueado
} PlayerProcess;

typedef struct {
    unsigned short width; // Ancho del tablero
    unsigned short height; // Alto del tablero
    unsigned int numOfPlayers; // Cantidad de jugadores
    PlayerProcess players[9]; // Lista de jugadores
    bool hasFinished; // Indica si el juego se ha terminado
    int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} Game;

typedef struct {
    sem_t printNeeded; // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t printFinished; // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t writerEntryMutex; // Mutex para evitar inanición del master al acceder al estado
    sem_t gameStateMutex; // Mutex para el estado del juego
    sem_t readersCountMutex; // Mutex para la siguiente variable
    unsigned int activeReaders; // Cantidad de jugadores leyendo el estado
} Semaphores;


int testSHM(char* name){
    printf("[TEST] Intentando conectar a la memoria compartida '%s'\n", name);

    // Paso 1: Abrir la memoria compartida
    int fd = shm_open(name, O_RDONLY, 0);
    if (fd == -1) {
        perror("[ERROR] shm_open");
        return 1;
    }

    // Paso 2: Obtener el tamaño con fstat()
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("[ERROR] fstat");
        close(fd);
        return 1;
    }

    size_t size = sb.st_size;
    printf("[TEST] Tamaño detectado: %ld bytes\n", size);

    // Paso 3: Hacer mmap
    char *ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("[ERROR] mmap");
        close(fd);
        return 1;
    }

    sem_post(&((Semaphores*)ptr)->printNeeded);
    // Paso 4: Mostrar contenido
    printf("[TEST] Contenido leído desde la memoria:\n");
    write(STDOUT_FILENO, ptr, size);
    printf("\n");
    sem_wait(&((Semaphores*)ptr)->printFinished);

    // Paso 5: Limpieza
    munmap(ptr, size);
    close(fd);

    printf("[TEST] Conexión exitosa ✅\n");
}


int main(int argc, char* argv[]) {
    testSHM("/game_sync");
    // testSHM("/game_state");

    printf("nigga link activated\n");
    for (int i = 0; i < argc; i++) {
        printf(" arg %d %s\n",i, argv[i]);
    }
    return 0;
}

