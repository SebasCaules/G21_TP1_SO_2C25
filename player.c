#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

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

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"

int main() {

    // Abrir la primera shm
    int fdState = shm_open(SHM_STATE, O_RDONLY, 0);
    if (fdState == -1) {
        perror("shm_open shm_state");
        exit(EXIT_FAILURE);
    }

    // Abrir la segunda shm
    int fdSync = shm_open(SHM_SYNC, O_RDWR, 0);
    if (fdSync == -1) {
        perror("shm_open shm_sync");
        exit(EXIT_FAILURE);
    }

    // Obtener tamaño de cada shm
    struct stat stateState, statSync;
    fstat(fdState, &stateState);
    fstat(fdSync, &statSync);

    // Mapear ambas
    Game *state = mmap(NULL, stateState.st_size, PROT_READ, MAP_SHARED, fdState, 0);
    if (state == MAP_FAILED) {
        perror("mmap shm_state");
        exit(EXIT_FAILURE);
    }

    Semaphores *sync = mmap(NULL, statSync.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdSync, 0);
    if (sync == MAP_FAILED) {
        perror("mmap shm_sync");
        exit(EXIT_FAILURE);
    }

    // Limpieza
    munmap(state, stateState.st_size);
    munmap(sync, statSync.st_size);
    close(fdState);
    close(fdSync);

    return 0;
}