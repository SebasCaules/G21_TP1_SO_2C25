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
    const char *bodyColors[] = {
        "\033[31m", // Red
        "\033[32m", // Green
        "\033[33m", // Yellow
        "\033[34m", // Blue
        "\033[35m", // Magenta
        "\033[36m", // Cyan
        "\033[1;31m", // Bold Red (distinct from normal red)
        "\033[1;32m", // Bold Green (distinct from normal green)
        "\033[1;33m"  // Bold Yellow (distinct from normal yellow)
    };
    const char *headColors[] = {
        "\033[97;41m", // White text on red background
        "\033[97;42m", // White text on green background
        "\033[97;43m", // White text on yellow background
        "\033[97;44m", // White text on blue background
        "\033[97;45m", // White text on magenta background
        "\033[97;46m", // White text on cyan background
        "\033[97;101m", // White text on bold red background
        "\033[97;102m", // White text on bold green background
        "\033[97;103m"  // White text on bold yellow background
    };
    const char *reset = "\033[0m"; // Reset color

    printf("📋 Tablero (%dx%d) | %d jugadores\n", width, height, state->numOfPlayers);
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            int value = state->board[x * width + y];

            if (value <= 0) {
                int playerIndex = -value;

                bool isHead = false;
                for (int i = 0; i < state->numOfPlayers; i++) {
                    if (state->players[i].x == y && state->players[i].y == x && playerIndex == i) {
                        isHead = true;
                        break;
                    }
                }

                if (isHead) {
                    printf("%s%4d%s", headColors[playerIndex % 9], value, reset);
                } else {
                    printf("%s%4d%s", bodyColors[playerIndex % 9], value, reset);
                }
            } else {
                printf("%4d", value);
            }
        }
        printf("\n");
    }
}

void printPlayerData(PlayerState player) {
    printf("Jugador: %-16s | Puntaje: %6d | Inválidos: %3d | Válidos: %3d | Posición: (%2d, %2d) %-3s\n",
           player.playerName, 
           player.score, 
           player.requestedValidMovements,
           player.requestedInvalidMovements, 
           player.x, player.y,
           player.isBlocked ? "| Bloqueado" : "");
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

    // printf("%d x %d y\n", state->players[0].x, state->players[0].y);
    // printf("%d x %d y\n", state->players[1].x, state->players[1].y);
    
    // sleep(5);

    while (!state->hasFinished) {
        sem_wait(&sync->printNeeded);
        printf("\033[H\033[J");
        for (int i = 0; i < state->numOfPlayers; i++) {
            printPlayerData(state->players[i]);
            printf("\n");
        }
        printBoard(state, height, width);
        sem_post(&sync->printFinished);
    }


    // Limpieza
    munmap(state, stateState.st_size);
    munmap(sync, statSync.st_size);
    close(fdState);
    close(fdSync);


    return 0;
}