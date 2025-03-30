#include <stdbool.h>
#include <stdio.h>
#include "shmlib.h"
#include "model.h"
#include <time.h>
#include <string.h>
#include <math.h>

void fillBoard(GameState *state, unsigned int seed);

void addPlayerToBoard(GameState *state);

void callView(Semaphores *sync);

void callPlayer(Semaphores *sync);

void eraseSHM(char *name);

void procesar_movimiento(pid_t jugadorPid, unsigned char moveRequest, GameState *state);

bool todos_los_jugadores_bloqueados(GameState *state);

void distributePlayers(GameState *state);

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
    
    // Initialize the game state and semaphores here (not implemented yet)
    GameState *state = (GameState *)createSHM(SHM_STATE, sizeof(GameState) + width * height * sizeof(int), 0644);
    Semaphores *sync = (Semaphores *)createSHM(SHM_SYNC, sizeof(Semaphores), 0666);

    static const int xArr[9] = { 8, 8, 6, 3, 2, 2, 3, 6, 8 };
    static const int yArr[9] = { 5, 7, 8, 8, 6, 4, 2, 2, 3 };

    state->width = width;
    state->height = height;
    state->numOfPlayers = numPlayers;
    state->hasFinished = false;
    for (size_t i = 0; i < numPlayers; i++) {
        state->players[i] = (PlayerState){
            .score = 0,
            .x = xArr[i] * (int) height/10,
            .y = yArr[i] * (int) width/10,
            .requestedInvalidMovements = 0,
            .requestedValidMovements = 0,
            .isBlocked = false
        };
        snprintf(state->players[i].playerName, sizeof(state->players[i].playerName), "%s (%d)", playerPaths[i] + 2, (int)i);
    }
    distributePlayers(state);
    fillBoard(state, seed);

    // Initialize semaphores
    sem_init(&sync->printNeeded, 1, 0);
    sem_init(&sync->printFinished, 1, 0);
    sem_init(&sync->writerEntryMutex, 1, 1);
    sem_init(&sync->gameStateMutex, 1, 1);
    sem_init(&sync->readersCountMutex, 1, 1);
    sync->activeReaders = 0;

    char width_str[8], height_str[8];
    snprintf(width_str, sizeof(width_str), "%d", state->width);
    snprintf(height_str, sizeof(height_str), "%d", state->height);

    pid_t view_pid = fork();
    if(view_pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (view_pid == 0) {
        char *args[] = { "./view", width_str, height_str, NULL };
        execv("./view", args);
        perror("execv view");
        exit(EXIT_FAILURE);
    }

    int player_pipes[MAX_PLAYERS]; // Guardar file descriptors para leer de los jugadores

    for (int i = 0; i < numPlayers; i++) {
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

            char *args[] = { "./player", width_str, height_str, NULL };
            execv("./player", args);
            perror("execv player");
            exit(1);
        } else if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else {
            // PADRE - máster
            close(pipefd[1]); // No escribe, solo lee
            state->players[i].pid = pid;         // Guardamos el PID del jugador
            player_pipes[i] = pipefd[0];         // Guardamos el FD de lectura
        }
    }

    //Print first board
    callView(sync);
    // Marca el momento del último movimiento válido
    time_t lastValidMove = time(NULL);

    while (!state->hasFinished) {
        bool someoneDidAValidMove = false;

        // Recorrer a todos los jugadores para leer un posible movimiento
        for (int i = 0; i < numPlayers; i++) {
            // Si este jugador ya está bloqueado, no leemos
            if (state->players[i].isBlocked) {
                continue;
            }

            // Intentar leer un byte (movimiento) del pipe
            unsigned char move;
            ssize_t bytesRead = read(player_pipes[i], &move, sizeof(move));

            if (bytesRead == sizeof(move)) {
                // Guardar la cantidad de movimientos válidos antes de procesar
                unsigned int validBefore = state->players[i].requestedValidMovements;

                // Aplica la lógica de ChompChamps
                procesar_movimiento(state->players[i].pid, move, state);

                // Si subió la cantidad de movimientos válidos, hubo movimiento real
                if (state->players[i].requestedValidMovements > validBefore) {
                    someoneDidAValidMove = true;
                    lastValidMove = time(NULL);
                }
            }
            // Si no hay nada para leer, simplemente seguimos con el siguiente jugador
        }

        // Revisar si todos los jugadores están bloqueados
        bool allBlocked = true;
        for (int i = 0; i < numPlayers; i++) {
            if (!state->players[i].isBlocked) {
                allBlocked = false;
                break;
            }
        }
        if (allBlocked) {
            // Fin: nadie más puede moverse
            state->hasFinished = true;
            callView(sync);
            break;
        }

        // Revisar si se superó el timeout sin movimientos válidos
        if (!someoneDidAValidMove) {
            time_t now = time(NULL);
            if (difftime(now, lastValidMove) >= timeout) {
                state->hasFinished = true; // Se terminó por inactividad
                callView(sync);
                break;
            }
        }
        callView(sync);

        // Pequeña pausa para no quemar CPU
        usleep(delay * 1000);
    }

    int bestPlayer = 0;

    for (size_t i = 0; i < state->numOfPlayers; i++) {
        printf("Player %s exited (%d) with score of %d / %d / %d\n", 
        state->players[i], 0, state->players[i].score,
        state->players[i].requestedValidMovements,
        state->players[i].requestedInvalidMovements);
        if (state->players[i].score > state->players[bestPlayer].score) {
            bestPlayer = i;
        }
        if (state->players[i].score == state->players[bestPlayer].score) {
            if (state->players[i].requestedValidMovements < state->players[bestPlayer].requestedValidMovements) {
                bestPlayer = i;
            }
        }
    }
    printf("---------------------------------------------------\n");
    printf("Player %s (%d) is the winner with a score of %d\n", state->players[bestPlayer].playerName, bestPlayer, state->players[bestPlayer].score);
    printf("Juego terminado\n");
    
    //Borrar SHM al finalizar
    eraseSHM(SHM_STATE);
    eraseSHM(SHM_SYNC);

    
    // falta cerrar pipes?


    return 0;   
}

void distributePlayers(GameState *state) {
    float centerX = 8.0f;
    float centerY = 5.0f;
    float radius = 3.0f;

    int numPlayers = state->numOfPlayers;
    int width = state->width;
    int height = state->height;

    for (int i = 0; i < numPlayers; i++) {
        // Ángulo para cada jugador
        float angle = (2.0f * M_PI * i) / (float)numPlayers;

        float fx = centerX + radius * cosf(angle);
        float fy = centerY + radius * sinf(angle);

        // Redondear
        int x = (int)lroundf(fx);
        int y = (int)lroundf(fy);

        // Clamp seguro (garantiza [0, width-1] y [0, height-1])
        if (x < 0) x = 0;
        if (x >= width) x = width - 1;
        if (y < 0) y = 0;
        if (y >= height) y = height - 1;

        state->players[i].x = x;
        state->players[i].y = y;
    }
}


bool todos_los_jugadores_bloqueados(GameState *state) {
    for (int i = 0; i < state->numOfPlayers; i++) {
        if (!state->players[i].isBlocked) {
            return false; // Al menos uno sigue activo
        }
    }
    return true;
}

void procesar_movimiento(pid_t jugadorPid, unsigned char moveRequest, GameState *state) {
    PlayerState *player = NULL;
    for (int i = 0; i < state->numOfPlayers; i++) {
        if (state->players[i].pid == jugadorPid) {
            player = &state->players[i];
            break;
        }
    }
    if (!player || player->isBlocked) return;

    int dx[] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int dy[] = { -1, -1,  0,  1,  1,  1,  0, -1 };

    int new_x = player->x + dx[moveRequest % 8];
    int new_y = player->y + dy[moveRequest % 8];

    bool dentroDelTablero =
        new_x >= 0 && new_x < state->width &&
        new_y >= 0 && new_y < state->height;

    bool celdaLibre = false;
    int valorCelda = 0;

    if (dentroDelTablero) {
        valorCelda = state->board[new_y * state->width + new_x];
        // Ahora solo permitimos celdas con valor estrictamente > 0
        if (valorCelda > 0) {
            celdaLibre = true;
        }
    }

    bool movimiento_valido = dentroDelTablero && celdaLibre;

    if (movimiento_valido) {
        player->score += valorCelda;
        player->x = new_x;
        player->y = new_y;

        int index = (int)(player - state->players);
        state->board[new_y * state->width + new_x] = -index;

        player->requestedValidMovements++;
    } else {
        player->requestedInvalidMovements++;
    }

    // Verificar si el jugador está completamente bloqueado
    bool puede_moverse = false;
    for (int d = 0; d < 8; d++) {
        int tx = player->x + dx[d];
        int ty = player->y + dy[d];
        if (tx >= 0 && tx < state->width &&
            ty >= 0 && ty < state->height &&
            state->board[ty * state->width + tx] > 0) {
            puede_moverse = true;
            break;
        }
    }

    if (!puede_moverse) {
        player->isBlocked = true;
    }
}

void callPlayer(Semaphores *sync) {
    sem_post(&sync->printNeeded);
    sem_wait(&sync->printFinished);
}

void callView(Semaphores *sync) {
    sem_post(&sync->printNeeded);
    sem_wait(&sync->printFinished);
}

void fillBoard(GameState *state, unsigned int seed) {
    srand(seed);
    for (int y = 0; y < state->height; y++) {
        for (int x = 0; x < state->width; x++) {
            state->board[y * state->width + x] = rand() % 9 + 1;
        }
    }
}

void addPlayerToBoard(GameState *state) {
    for (int i = 0; i < state->numOfPlayers; i++) {
        state->board[state->players[i].y * state->width + state->players[i].x] = -i;
    }
}