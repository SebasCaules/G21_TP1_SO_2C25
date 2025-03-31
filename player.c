#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "shmlib.h"
#include "model.h"

void calculateNewPosition(unsigned char moveRequest, unsigned short *newX, unsigned short *newY, unsigned short x, unsigned short y);

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
        moveRequest = rand() % 8;
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
        usleep(100000); // ?
    }

    closeSHM(state, stateSize);
    closeSHM(sync, syncSize);

    return 0;
}

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
