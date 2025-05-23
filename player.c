#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "shmlib.h"
#include "model.h"
#include <math.h>
#include <unistd.h>
#include "validateLib.h"

unsigned char calculateBestMove(PlayerState *player, GameState *state);
int adjacentFreeCells(GameState *state, unsigned short x, unsigned short y);
int riskFromEnemies(GameState *state, unsigned short x, unsigned short y);
unsigned int calculateDistanceToWall(GameState *state, unsigned short x, unsigned short y);
int evaluateCell(GameState *state, unsigned short x, unsigned short y);

int main(int agrc, char *argv[]) {

    srand(time(NULL) ^ getpid());

    size_t stateSize, syncSize;

    GameState* state = (GameState*) openSHM(SHM_STATE, O_RDONLY, &stateSize);
    Semaphores* sync = (Semaphores*) openSHM(SHM_SYNC, O_RDWR, &syncSize);

    while (!state->hasFinished) {
        sem_wait(&sync->readersCountMutex);
        sync->activeReaders++;
        if (sync->activeReaders == 1) {
            sem_wait(&sync->writerEntryMutex);
        }
        sem_post(&sync->readersCountMutex);
        sem_wait(&sync->gameStateMutex);
        PlayerState *player = NULL;
        pid_t currentPid = getpid();
        for (int i = 0; i < state->numOfPlayers; i++) {
            if (state->players[i].pid == currentPid) {
                player = &state->players[i];
                break;
            }
        }
        if (player == NULL) {
            fprintf(stderr, "Error: No player found for process with PID %d\n", currentPid);
            sem_post(&sync->gameStateMutex);
            exit(EXIT_FAILURE);
        }
        unsigned char moveRequest;
        moveRequest = calculateBestMove(player, state);
        if (write(1, &moveRequest, sizeof(moveRequest)) == -1) {
            perror("write to pipe");
            player->isBlocked = true;
            sem_post(&sync->gameStateMutex);
            break;
        }
        sem_post(&sync->gameStateMutex);
        sem_wait(&sync->readersCountMutex);
        sync->activeReaders--;
        if (sync->activeReaders == 0) {
            sem_post(&sync->writerEntryMutex);
        }
        sem_post(&sync->readersCountMutex);
        usleep(150000);
    }
    closeSHM(state, stateSize);
    closeSHM(sync, syncSize);
    return 0;
}

unsigned char calculateBestMove(PlayerState *player, GameState *state) {
    int bestScore = -1;
    int bestDirection = -1;

    for(unsigned char dir = 0; dir < 8; dir++) {
        int newX, newY;
        positionAfterMove(dir, &newX, &newY, player->x, player->y);
        if(isValid(state, newX, newY)) {
            int score = evaluateCell(state, newX, newY);
            if(score > bestScore) {
                bestScore = score;
                bestDirection = dir;
            }
        }
    }
    return bestDirection;
}

int adjacentFreeCells(GameState *state, unsigned short x, unsigned short y) {
    int count = 0;
    int newX, newY;

    for(unsigned char dir = 0; dir < 8; dir++) {
        positionAfterMove(dir, &newX, &newY, x, y);
        if(isValid(state, newX, newY)) {
            count++;
        }
    }
    return count;
}

int riskFromEnemies(GameState *state, unsigned short x, unsigned short y) {
    int risk = 0;
    PlayerState enemy;
    int distance;

    for(int i = 0; i < state->numOfPlayers; i++) {
        enemy = state->players[i];
        distance = fmax(fabs(x - enemy.x), fabs(y - enemy.y));
        //si enemy es el mismo player no pasa nada, distance dara 0
        if(distance == 1) {
            risk += 2;
        } else if(distance == 2) {
            risk +=1;
        }
    }
    return risk;
}

unsigned int calculateDistanceToWall(GameState *state, unsigned short x, unsigned short y) {
    return fmin(fmin(fmin(x, y), state->width - x), state->height - y);
}

int evaluateCell(GameState *state, unsigned short x, unsigned short y) {
    int reward = state->board[y * state->width + x];
    int freeSpaces = adjacentFreeCells(state, x, y);
    int distanceToWall = calculateDistanceToWall(state, x, y);
    int risk = riskFromEnemies(state, x, y);
    return reward * 5 + freeSpaces * 2 - distanceToWall - risk;
}

