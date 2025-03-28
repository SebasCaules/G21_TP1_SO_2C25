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





