#include <stdbool.h>
#include <stdio.h>
#include "shmlib.h"
#include "model.h"

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