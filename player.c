#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    char playerName[16];                    // Nombre del jugador
    unsigned int score;                     // Puntaje
    unsigned int requestedInvalidMovements; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int requestedValidMovements;   // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y;                    // Coordenadas x e y en el tablero
    pid_t pid;                              // Identificador de proceso
    bool isBlocked;                         // Indica si el jugador está bloqueado
} PlayerProcess;

typedef struct
{
    unsigned short width;      // Ancho del tablero
    unsigned short height;     // Alto del tablero
    unsigned int numOfPlayers; // Cantidad de jugadores
    PlayerProcess players[9];  // Lista de jugadores
    bool hasFinished;          // Indica si el juego se ha terminado
    int board[];               // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} Game;

typedef struct
{
    sem_t printNeeded;          // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t printFinished;        // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t writerEntryMutex;     // Mutex para evitar inanición del master al acceder al estado
    sem_t gameStateMutex;       // Mutex para el estado del juego
    sem_t readersCountMutex;    // Mutex para la siguiente variable
    unsigned int activeReaders; // Cantidad de jugadores leyendo el estado
} Semaphores;

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"

void calculateNewPosition(unsigned char moveRequest, unsigned short *newX, unsigned short *newY, unsigned short x, unsigned short y) {
    switch (moveRequest) {
        case 0: *newY = y - 1; *newX = x;     break; // Up
        case 1: *newY = y - 1; *newX = x + 1; break; // Up-Right
        case 2: *newY = y;     *newX = x + 1; break; // Right
        case 3: *newY = y + 1; *newX = x + 1; break; // Down-Right
        case 4: *newY = y + 1; *newX = x;     break; // Down
        case 5: *newY = y + 1; *newX = x - 1; break; // Down-Left
        case 6: *newY = y;     *newX = x - 1; break; // Left
        case 7: *newY = y - 1; *newX = x - 1; break; // Up-Left
    }
}

int main()
{

    srand(time(NULL));

    // Abrir la primera shm
    int fdState = shm_open(SHM_STATE, O_RDONLY, 0);
    if (fdState == -1)
    {
        perror("shm_open shm_state");
        exit(EXIT_FAILURE);
    }

    // Abrir la segunda shm
    int fdSync = shm_open(SHM_SYNC, O_CREAT | O_RDWR, 0666);
    if (fdSync == -1)
    {
        perror("shm_open shm_sync");
        exit(EXIT_FAILURE);
    }

    // Obtener tamaño de cada shm
    struct stat stateState, statSync;
    fstat(fdState, &stateState);
    fstat(fdSync, &statSync);

    // Mapear ambas
    Game *state = mmap(NULL, stateState.st_size, PROT_READ, MAP_SHARED, fdState, 0);
    if (state == MAP_FAILED)
    {
        perror("mmap shm_state");
        exit(EXIT_FAILURE);
    }

    Semaphores *sync = mmap(NULL, statSync.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdSync, 0);
    if (sync == MAP_FAILED)
    {
        perror("mmap shm_sync");
        exit(EXIT_FAILURE);
    }


    // Continuous loop for player movement
    while (!state->hasFinished) {
        // Lock the game state to find the player
        sem_wait(&sync->gameStateMutex);

        // Find the player controlled by this process
        PlayerProcess *player = NULL;
        pid_t currentPid = getpid();
        for (int i = 0; i < state->numOfPlayers; i++) {
            if (state->players[i].pid == currentPid) {
                player = &state->players[i];
                break;
            }
        }

        if (player == NULL) {
            fprintf(stderr, "Error: No player found for process with PID %d\n", currentPid);
            sem_post(&sync->gameStateMutex); // Unlock the game state
            exit(EXIT_FAILURE);
        }

        sem_post(&sync->gameStateMutex); // Unlock the game state

        // Generate a random move request (0-7)
        unsigned char moveRequest = rand() % 8;

        // Send the move request to the master via pipe (write to fd 1)
        if (write(1, &moveRequest, sizeof(moveRequest)) == -1) {
            perror("write to pipe");
            player->isBlocked = true; // Mark player as blocked if pipe fails
            break;
        }

        // Notify the view that the board has been updated
        sem_post(&sync->printNeeded);

        // Wait for the view to finish printing
        sem_wait(&sync->printFinished);

        // Wait for some time before the next move
        usleep(1000000); // 500 ms delay
    }

    // Limpieza
    munmap(state, stateState.st_size);
    munmap(sync, statSync.st_size);
    close(fdState);
    close(fdSync);

    return 0;
}