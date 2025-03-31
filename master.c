#include <stdbool.h>
#include <stdio.h>
#include "shmlib.h"
#include "model.h"
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>
#include "colors.h"

void parseArguments(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, unsigned int *timeout, unsigned int *seed, char **viewPath, char *playerPaths[], int *numOfPlayers);
void getPlayerInitialPosition(int playerIndex, int numOfPlayers, int width, int height, int *x, int *y);
void fillBoard(GameState *state, unsigned int seed);
void processMovement(pid_t jugadorPid, unsigned char moveRequest, GameState *state);
bool areAllPlayersBlocked(GameState *state);
void callView(Semaphores *sync);
int comparePlayers(PlayerState *a, PlayerState *b);
void addPlayersToBoard(GameState *state);

int main(int argc, char *argv[]) {
    unsigned short width = WIDTH;
    unsigned short height = HEIGHT;
    unsigned int delay = DELAY;
    unsigned int timeout = TIMEOUT;
    unsigned int seed = time(NULL);
    char *viewPath = NULL;
    char *playerPaths[MAX_PLAYERS];
    int numOfPlayers = 0;

    parseArguments(argc, argv, &width, &height, &delay, &timeout, &seed, &viewPath, playerPaths, &numOfPlayers);
    sleep(2);

    GameState *state = (GameState *)createSHM(SHM_STATE, sizeof(GameState) + width * height * sizeof(int), 0644);
    Semaphores *sync = (Semaphores *)createSHM(SHM_SYNC, sizeof(Semaphores), 0666);

    state->width = width;
    state->height = height;
    state->numOfPlayers = numOfPlayers;
    state->hasFinished = false;

    for (size_t i = 0; i < numOfPlayers; i++) {
        int x, y;
        getPlayerInitialPosition(i, numOfPlayers, width, height, &x, &y);
        state->players[i] = (PlayerState){
            .score = 0,
            .x = x,
            .y = y,
            .requestedInvalidMovements = 0,
            .requestedValidMovements = 0,
            .isBlocked = false
        };
        snprintf(state->players[i].playerName, sizeof(state->players[i].playerName), "%s (%d)", playerPaths[i] + 2, (int)i);
    }

    sem_init(&sync->printNeeded, 1, 0);
    sem_init(&sync->printFinished, 1, 0);
    sem_init(&sync->writerEntryMutex, 1, 1);
    sem_init(&sync->gameStateMutex, 1, 1);
    sem_init(&sync->readersCountMutex, 1, 1);
    sync->activeReaders = 0;

    // width and height int to string
    char width_str[8], height_str[8];
    snprintf(width_str, sizeof(width_str), "%d", state->width);
    snprintf(height_str, sizeof(height_str), "%d", state->height);

    pid_t view_pid = fork();
    if(view_pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (view_pid == 0) {
        char *args[] = { VIEW_PATH, width_str, height_str, NULL };
        execv(VIEW_PATH, args);
        perror("execv view");
        exit(EXIT_FAILURE);
    }

    int player_pipes[MAX_PLAYERS]; // Guardar file descriptors para leer de los jugadores

    for (int i = 0; i < numOfPlayers; i++) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == 0) {
            // HIJO - jugador
            close(pipefd[0]); // Cerramos lectura
            dup2(pipefd[1], STDOUT_FILENO); // Redirigimos stdout al pipe
            close(pipefd[1]); // Cerramos el original (ya está duplicado)

            char *args[] = { playerPaths[i], width_str, height_str, NULL };
            execv(playerPaths[i], args);
            perror("execv player");
            exit(EXIT_FAILURE);
        } else if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else {
            // PADRE - máster
            close(pipefd[1]); // No escribe, solo lee
            state->players[i].pid = pid; // Guardamos el PID del jugador
            player_pipes[i] = pipefd[0]; // Guardamos el FD de lectura
        }
    }

    fillBoard(state, seed);
    addPlayersToBoard(state);
    // Imprime el tablero inicial
    callView(sync);

    time_t lastValidMove = time(NULL);
    // Loop principal del juego
    while (!state->hasFinished) {
        for (int i = 0; i < numOfPlayers; i++) {
            sem_wait(&sync->gameStateMutex);
            if (state->players[i].isBlocked) {
                sem_post(&sync->gameStateMutex);
                continue;
            }
            sem_post(&sync->gameStateMutex);
            callView(sync);
            unsigned char move;
            ssize_t bytesRead = read(player_pipes[i], &move, sizeof(move));
            if (bytesRead == sizeof(move)) {
                sem_wait(&sync->gameStateMutex);
                unsigned int validBefore = state->players[i].requestedValidMovements;
                processMovement(state->players[i].pid, move, state);
                if (state->players[i].requestedValidMovements > validBefore) {
                    lastValidMove = time(NULL);
                }
                sem_post(&sync->gameStateMutex);
            }
            sem_wait(&sync->gameStateMutex);
            bool allBlocked = areAllPlayersBlocked(state);
            sem_post(&sync->gameStateMutex);
            if (allBlocked) {
                state->hasFinished = true;
            }
            // Chequea si se acabó el tiempo
            time_t now = time(NULL);
            if (difftime(now, lastValidMove) >= timeout) {
                state->hasFinished = true;
                
            }
            usleep(delay * 1000);
        }
        if (state->hasFinished) {
            callView(sync);
            break;
        }
        callView(sync);
    }

    int status;
    pid_t pid = waitpid(view_pid, &status, 0);
    if (pid == -1) {
        perror("waitpid for view process");
    } else if (WIFEXITED(status)) {
        printf("View exited (%d)\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("View process was terminated by signal %d\n", WTERMSIG(status));
    } else {
        printf("View process terminated abnormally\n");
    }
    
    int winnerIndex = 0;
    int drawIndex[MAX_PLAYERS];
    int drawCount = 0;
    for (int i = 0; i < numOfPlayers; i++) {
        pid_t pid = waitpid(state->players[i].pid, &status, 0);
        if (pid == -1) {
            perror("waitpid");
            continue;
        }
        char *playerName = state->players[i].playerName;
        const char *color = bodyColors[i];
        if (WIFEXITED(status)) {
            printf("Player%s %s %sexited (%d) with score of %d / %d / %d\n", 
                color, playerName, reset, WEXITSTATUS(status), state->players[i].score,
                state->players[i].requestedValidMovements,state->players[i].requestedInvalidMovements);
            // Comparacion de puntaje para sacar al ganador
            int cmp = comparePlayers(&state->players[i], &state->players[winnerIndex]);
            if (cmp < 0) {
                winnerIndex = i;
                drawCount = 1;
                drawIndex[0] = i;
            } else if (cmp == 0) {
                if (i == winnerIndex) continue;
                if (drawCount == 0) {
                    drawIndex[0] = winnerIndex;
                    drawCount = 1;
                }
                drawIndex[drawCount++] = i;
            }
            // si cmp > 0 => el actual sigue siendo mejor
        } else if (WIFSIGNALED(status)) {
            printf("Player%s %s %skilled by signal %d\n", 
                color, playerName, reset,
                WTERMSIG(status));
        } else {
            printf("Player%s %s %sterminated abnormally\n", 
                color, playerName, reset);
        }
        // Cerramos el pipe de lectura
        close(player_pipes[i]);
    }

    printf("---------------------------------------------------------\n");

    if (drawCount > 1) {
        printf("Players ");
        for (int i = 0; i < drawCount; i++) {
            printf("%s%s%s", bodyColors[drawIndex[i]],state->players[drawIndex[i]].playerName, reset);
            if (i == drawCount - 2) printf(" and ");
            else if (i < drawCount - 2) printf(", ");
            }
            printf(" tied with a score of %d / %d / %d\n", state->players[winnerIndex].score,
                state->players[winnerIndex].requestedValidMovements,
                state->players[winnerIndex].requestedInvalidMovements);
    } else {
        printf("Player%s %s %sis the winner with a score of %d\n", bodyColors[winnerIndex],
            state->players[winnerIndex].playerName, reset, state->players[winnerIndex].score);
    }

    // Cerramos semáforos
    sem_destroy(&sync->printNeeded);
    sem_destroy(&sync->printFinished);
    sem_destroy(&sync->writerEntryMutex);
    sem_destroy(&sync->gameStateMutex);
    sem_destroy(&sync->readersCountMutex);
    
    // Borramos memorias compartidas
    eraseSHM(SHM_STATE);
    eraseSHM(SHM_SYNC);
    return 0;   
}

void parseArguments(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, unsigned int *timeout, unsigned int *seed, char **viewPath, char *playerPaths[], int *numOfPlayers) {
    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w': // Width
                *width = atoi(optarg);
                break;
            case 'h': // Height
                *height = atoi(optarg);
                break;
            case 'd': // Delay
                *delay = atoi(optarg);
                break;
            case 't': // Timeout
                *timeout = atoi(optarg);
                break;
            case 's': // Seed
                *seed = atoi(optarg);
                break;
            case 'v': // View path
                *viewPath = optarg;
                break;
            case 'p': // Player paths
                if (*numOfPlayers >= MAX_PLAYERS) {
                    fprintf(stderr, "Maximum number of players is %d.\n", MAX_PLAYERS);
                    exit(EXIT_FAILURE);
                }
                playerPaths[(*numOfPlayers)++] = optarg;
                while (optind < argc && argv[optind][0] != '-') {
                    if (*numOfPlayers >= MAX_PLAYERS) {
                        fprintf(stderr, "Maximum number of players is %d.\n", MAX_PLAYERS);
                        exit(EXIT_FAILURE);
                    }
                    playerPaths[(*numOfPlayers)++] = argv[optind++];
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (*width < WIDTH || *height < HEIGHT) {
        fprintf(stderr, "Error: Minimal board dimensions: %dx%d. Given %dx%d.\n", WIDTH, HEIGHT, *width, *height);
        exit(EXIT_FAILURE);
    }

    if (*numOfPlayers < 1) {
        fprintf(stderr, "Error: At least one player must be provided.\n");
        exit(EXIT_FAILURE);
    }

    printf("\033[H\033[J");
    printf("width: %d\n", *width);
    printf("height: %d\n", *height);
    printf("delay: %d ms\n", *delay);
    printf("timeout: %d\n", *timeout);
    printf("seed: %d\n", *seed);
    printf("view: %s\n", *viewPath);
    printf("num_players: %d\n", *numOfPlayers);
    for (int i = 0; i < *numOfPlayers; i++) {
        printf("  %s\n", playerPaths[i]);
    }
}

void getPlayerInitialPosition(int playerIndex, int numOfPlayers, int width, int height, int *x, int *y) {
    int cols = (int)ceil(sqrt(numOfPlayers));
    int rows = (int)ceil((float)numOfPlayers / cols);

    int regionWidth = width / cols;
    int regionHeight = height / rows;

    int row = playerIndex / cols;
    int col = playerIndex % cols;

    int centerX = col * regionWidth + regionWidth / 2;
    int centerY = row * regionHeight + regionHeight / 2;

    if (centerX >= width) centerX = width - 1;
    if (centerY >= height) centerY = height - 1;

    *x = centerX;
    *y = centerY;
}

void fillBoard(GameState *state, unsigned int seed) {
    srand(seed);
    for (int y = 0; y < state->height; y++) {
        for (int x = 0; x < state->width; x++) {
            state->board[y * state->width + x] = rand() % 9 + 1;
        }
    }
}

void processMovement(pid_t playerPID, unsigned char moveRequest, GameState *state) {
    PlayerState *player = NULL;
    for (int i = 0; i < state->numOfPlayers; i++) {
        if (state->players[i].pid == playerPID) {
            player = &state->players[i];
            break;
        }
    }
    if (!player || player->isBlocked) return;

    int dx[] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int dy[] = { -1, -1,  0,  1,  1,  1,  0, -1 };

    int newX = player->x + dx[moveRequest % 8];
    int newY = player->y + dy[moveRequest % 8];

    bool isWithinBounds =
        newX >= 0 && newX < state->width &&
        newY >= 0 && newY < state->height;

    bool isCellFree = false;
    int cellValue = 0;

    if (isWithinBounds) {
        cellValue = state->board[newY * state->width + newX];
        if (cellValue > 0) {
            isCellFree = true;
        }
    }

    bool isValidMove = isWithinBounds && isCellFree;

    if (isValidMove) {
        player->score += cellValue;
        player->x = newX;
        player->y = newY;

        int index = (int)(player - state->players);
        state->board[newY * state->width + newX] = -index;

        player->requestedValidMovements++;
    } else {
        player->requestedInvalidMovements++;
    }

    bool canMove = false;
    for (int d = 0; d < 8; d++) {
        int tx = player->x + dx[d];
        int ty = player->y + dy[d];
        if (tx >= 0 && tx < state->width && ty >= 0 && ty < state->height && 
            state->board[ty * state->width + tx] > 0) {
            canMove = true;
            break;
        }
    }

    if (!canMove) {
        player->isBlocked = true;
    }
}

bool areAllPlayersBlocked(GameState *state) {
    for (int i = 0; i < state->numOfPlayers; i++) {
        if (!state->players[i].isBlocked) {
            return false; // Al menos uno sigue activo
        }
    }
    return true;
}

void callView(Semaphores *sync) {
    sem_post(&sync->printNeeded);
    sem_wait(&sync->printFinished);
}

int comparePlayers(PlayerState *a, PlayerState *b) {
    if (a->score != b->score) return (a->score > b->score) ? -1 : 1;
    if (a->requestedValidMovements != b->requestedValidMovements)
        return (a->requestedValidMovements < b->requestedValidMovements) ? -1 : 1;
    if (a->requestedInvalidMovements != b->requestedInvalidMovements)
        return (a->requestedInvalidMovements < b->requestedInvalidMovements) ? -1 : 1;
    return 0; // empate exacto
}

void addPlayersToBoard(GameState *state) {
    for (int i = 0; i < state->numOfPlayers; i++) {
        state->board[state->players[i].y * state->width + state->players[i].x] = -i;
    }
}
