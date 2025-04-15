#include "validateLib.h"

int dx[] = {  0,  1,  1,  1,  0, -1, -1, -1 };
int dy[] = { -1, -1,  0,  1,  1,  1,  0, -1 };

bool isValid(GameState *state, int x, int y) {

    //checkea si esta dentro del tablero
    bool isWithinBounds =
        x >= 0 && x < state->width &&
        y >= 0 && y < state->height;

    if (!isWithinBounds) {
        return false;
    }

    //checkea que lo que hay en la casilla no es un jugador
    int cellValue = state->board[y * state->width + x];
    return cellValue > 0;
}

int directionX(unsigned char moveRequest) {
    return dx[moveRequest % 8];
}

int directionY(unsigned char moveRequest) {
    return dy[moveRequest % 8];
}

void directionBreakdown(unsigned char moveRequest, int *dx, int *dy) {
    *dx = directionX(moveRequest);
    *dy = directionY(moveRequest);
}

void positionAfterMove(unsigned char moveRequest, int *newX, int *newY, unsigned short x, unsigned short y) {
    *newX = x + directionX(moveRequest);
    *newY = y + directionY(moveRequest);
}