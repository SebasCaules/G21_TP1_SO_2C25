#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#define HEIGHT 10
#define WIDTH 10
#define DELAY 200
#define TIMEOUT 10
#define MAX_PLAYERS 9

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"

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

int main(int argc, char *argv[]) {
    // Default values
    unsigned short width = WIDTH;
    unsigned short height = HEIGHT;
    unsigned int delay = DELAY;
    unsigned int timeout = TIMEOUT;
    unsigned int seed = time(NULL);
    char *viewPath = NULL;
    char *playerPaths[MAX_PLAYERS];
    int numPlayers = 0;

    // Parse arguments
    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w': // Width
                width = atoi(optarg);
                break;
            case 'h': // Height
                height = atoi(optarg);
                break;
            case 'd': // Delay
                delay = atoi(optarg);
                break;
            case 't': // Timeout
                timeout = atoi(optarg);
                break;
            case 's': // Seed
                seed = atoi(optarg);
                break;
            case 'v': // View path
                viewPath = optarg;
                break;
            case 'p': // Player paths
                if (numPlayers >= MAX_PLAYERS) {
                    fprintf(stderr, "Maximum number of players is %d.\n", MAX_PLAYERS);
                    exit(EXIT_FAILURE);
                }
                playerPaths[numPlayers++] = optarg;
                while (optind < argc && argv[optind][0] != '-') {
                    if (numPlayers >= MAX_PLAYERS) {
                        fprintf(stderr, "Maximum number of players is %d.\n", MAX_PLAYERS);
                        exit(EXIT_FAILURE);
                    }
                    playerPaths[numPlayers++] = argv[optind++];
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (width < WIDTH || height < HEIGHT) {
        fprintf(stderr, "Error: Minimal board dimensions: %dx%d. Given %dx%d.\n", WIDTH, HEIGHT, width, height);
        exit(EXIT_FAILURE);
    }

    if (numPlayers < 1) {
        fprintf(stderr, "Debe proveer al menos un jugador.\n");
        exit(EXIT_FAILURE);
    }

    // Print parsed arguments for debugging
    printf("Parsed Arguments:\n");
    printf("Width: %d\n", width);
    printf("Height: %d\n", height);
    printf("Delay: %d ms\n", delay);
    printf("Timeout: %d s\n", timeout);
    printf("Seed: %u\n", seed);
    if (viewPath) {
        printf("View Path: %s\n", viewPath);
    } else {
        printf("View Path: None\n");
    }
    printf("Players (%d):\n", numPlayers);
    for (int i = 0; i < numPlayers; i++) {
        printf("  Player %d: %s\n", i + 1, playerPaths[i]);
    }

    // Initialize the game state and semaphores here (not implemented yet)

    return 0;
    
}

