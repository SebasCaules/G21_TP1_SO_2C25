#include <stdbool.h>
#include <stdio.h>
#include "shmlib.h"
#include "model.h"
#include "colors.h"

void printBoard(GameState *state, int height, int width);
void printPlayerData(PlayerState player, int index);

int main(int argc, char *argv[]) {

    int height = atoi(argv[1]);
    int width = atoi(argv[2]);
    
    size_t stateSize, syncSize;

    GameState* state = (GameState*) openSHM(SHM_STATE, O_RDONLY, &stateSize);
    Semaphores* sync = (Semaphores*) openSHM(SHM_SYNC, O_RDWR, &syncSize);

    while (!state->hasFinished) {
        sem_wait(&sync->printNeeded);
        printf("\033[H\033[J");
        for (int i = 0; i < state->numOfPlayers; i++) {
            printPlayerData(state->players[i], i);
            printf("\n");
        }
        printBoard(state, height, width);
        sem_post(&sync->printFinished);
    }

    closeSHM(state, stateSize);
    closeSHM(sync, syncSize);

    return 0;
}

void printBoard(GameState *state, int height, int width) {
    printf("Tablero (%dx%d) | %d jugadores\n", width, height, state->numOfPlayers);
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
                    printf("%s%3d%s", headColors[playerIndex % 9], value, reset);
                } else {
                    printf("%s%3d%s", bodyColors[playerIndex % 9], value, reset);
                }
            } else {
                printf("%3d", value);
            }
        }
        printf("\n");
    }
}

void printPlayerData(PlayerState player, int index) {
    // Print the player's name in its corresponding color
    printf("Jugador: %s%-16s%s | Puntaje: %6d | Inválidos: %3d | Válidos: %3d | Posición: (%2d, %2d) %-3s\n",
           bodyColors[index % 9], // Color for the player's name
           player.playerName,     // Player name
           reset,                 // Reset color after the name
           player.score,          // Player score
           player.requestedInvalidMovements, // Invalid movements
           player.requestedValidMovements,   // Valid movements
           player.x,              // X coordinate
           player.y,              // Y coordinate
           player.isBlocked ? "| Bloqueado" : ""); // Blocked status
}
