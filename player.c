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

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        return 1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t state_size = sizeof(GameState) + sizeof(int) * width * height;

    int fd_state = shm_open("/game_state", O_RDWR, 0);
    if (fd_state == -1) {
        perror("shm_open /game_state");
        return 1;
    }

    GameState *state = mmap(NULL, state_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_state, 0);
    if (state == MAP_FAILED) {
        perror("mmap game_state");
        return 1;
    }

    int fd_sync = shm_open("/game_sync", O_RDWR, 0);
    if (fd_sync == -1) {
        perror("shm_open /game_sync");
        return 1;
    }

    Semaphores *sync = mmap(NULL, sizeof(Semaphores), PROT_READ | PROT_WRITE, MAP_SHARED, fd_sync, 0);
    if (sync == MAP_FAILED) {
        perror("mmap game_sync");
        return 1;
    }

    pid_t my_pid = getpid();
    int id = -1;
    for (int i = 0; i < state->numOfPlayers; i++) {
        if (state->players[i].pid == my_pid) {
            id = i;
            break;
        }
    }

    if (id == -1) {
    for (int i = 0; i < state->numOfPlayers; i++) {
        printf("[DEBUG] Jugador %d → pid: %d\n", i, state->players[i].pid);
    }
    printf("[ERROR] Mi PID: %d. No estoy en la lista.\n", getpid());
    return 1;
}

    while (!state->hasFinished && !state->players[id].isBlocked) {
        int x = state->players[id].x;
        int y = state->players[id].y;

        if (x + 1 < state->width) {
            state->players[id].x += 1;
        } else {
            state->players[id].isBlocked = true;
        }

        state->players[id].requestedValidMovements++;
        state->board[y * state->width + x] = -id;

        sem_post(&sync->printNeeded);
        sem_wait(&sync->printFinished);

        sleep(1);
    }

    return 0;
}