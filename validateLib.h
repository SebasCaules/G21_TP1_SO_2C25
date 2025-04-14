
#include "model.h"

//Checkea si la posicion x,y es valida (no se va del tablero ni pisa un jugador)
bool isValid(GameState *state, int x, int y);

//Descompone la direccion en x e y
//Ejemplo: moveRequest = 3 (abajo a la derecha) -> dx = 1 (derecha) dy = -1 (abajo)
//utiliza directionX y directionY
void directionBreakdown(unsigned char moveRequest, int *dx, int *dy);

//Descompone la direccion en x
int directionX(unsigned char moveRequest);

//Descompone la direccion en y
int directionY(unsigned char moveRequest);


//Guarda en newX y newY la posicion del jugador que comienza en x,y y realiza el movimiento moveRequest
void positionAfterMove(unsigned char moveRequest, int *newX, int *newY, unsigned short x, unsigned short y);