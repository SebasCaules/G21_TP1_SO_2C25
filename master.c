#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HEIGHT 10
#define WIDTH 10

typedef struct {
    char playerName[16];
    unsigned int score;
    unsigned int requestedInvalidMoves;
    unsigned int requestedValidMoves;
    unsigned short x, y;
    // pid_t pid;
    bool availableValidMoves;
} Player;

typedef struct {
    int value;
    bool available;
} Cell;


int main(int agrc, char* argv[]) {
    init();
    print();
    return 0;
}


int mat[HEIGHT][WIDTH];

void init() {
    srand(time(NULL));

    // Llenar la matriz con valores random entre 0 y 9
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            mat[i][j] = rand() % 10;
        }
    }
}

void print() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

int placePlayer(Player * player) {
    
}
