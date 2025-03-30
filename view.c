#include <stdbool.h>
#include <stdio.h>
#include "shmlib.h"
#include "model.h"

int iterations = 0;

void printBoard(GameState *state, int height, int width) {
    printf("📋 Tablero (%dx%d) | %d jugadores\n", width, height, state->numOfPlayers);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int value = state->board[x * width + y]; // acceso lineal
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

    printf("Vista inicializada con %dx%d\n", width, height);

    size_t stateSize, syncSize;

    GameState* state = (GameState*) openSHM(SHM_STATE, O_RDONLY, &stateSize);
    Semaphores* sync = (Semaphores*) openSHM(SHM_SYNC, O_RDWR, &syncSize);

    printf("hola");

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

    closeSHM(state, stateSize);
    closeSHM(sync, syncSize);

    return 0;
}