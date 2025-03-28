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
} PlayerState;

typedef struct {
    unsigned short width; // Ancho del tablero
    unsigned short height; // Alto del tablero
    unsigned int numOfPlayers; // Cantidad de jugadores
    PlayerState players[9]; // Lista de jugadores
    bool hasFinished; // Indica si el juego se ha terminado
    int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;

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

int iterations = 0;

void printBoard(GameState *state, int height, int width) {
    printf("📋 Tablero (%dx%d):\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int value = state->board[y * width + x]; // acceso lineal
            printf("%4d", value);
        }
        printf("\n");
    }
}

void printPlayerData(PlayerState player) {
    printf("Jugador: %s\n", player.playerName);
    printf("Puntaje: %d\n", player.score);
    printf("Movimientos inválidos solicitados: %d\n", player.requestedInvalidMovements);
    printf("Movimientos válidos solicitados: %d\n", player.requestedValidMovements);
    printf("Coordenadas: (%d, %d)\n", player.x, player.y);
    printf("Bloqueado: %s\n", player.isBlocked ? "Sí" : "No");
}

int main(int argc, char *argv[]) {

    int height = atoi(argv[1]);
    int width = atoi(argv[2]);

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
    GameState *state = mmap(NULL, stateState.st_size, PROT_READ, MAP_SHARED, fdState, 0);
    if (state == MAP_FAILED) {
        perror("mmap shm_state");
        exit(EXIT_FAILURE);
    }

    Semaphores *sync = mmap(NULL, statSync.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdSync, 0);
    if (sync == MAP_FAILED) {
        perror("mmap shm_sync");
        exit(EXIT_FAILURE);
    }

    // Continuous loop to print the board
    while (!state->hasFinished) {
        // Wait for the signal that the board has been updated
        sem_wait(&sync->printNeeded);
        printf("\033[H\033[J"); // Clear the terminal
        for (int i = 0; i < state->numOfPlayers; i++) {
            printPlayerData(state->players[i]);
            printf("\n");
        }
        printBoard(state, height, width);
        
        // Signal that the printing is finished
        sem_post(&sync->printFinished);
    }

    // Limpieza
    munmap(state, stateState.st_size);
    munmap(sync, statSync.st_size);
    close(fdState);
    close(fdSync);


    return 0;
}