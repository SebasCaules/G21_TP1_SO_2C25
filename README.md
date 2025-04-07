# ChompChamps

ChompChamps es un juego concurrente por turnos, implementado en C, donde múltiples procesos jugadores compiten por capturar celdas de un tablero compartido. El juego utiliza memoria compartida POSIX y semáforos para sincronización entre procesos, y se ejecuta completamente en la terminal.

## Estructura del proyecto

.
├── master.c         # Proceso principal: coordina el juego, jugadores y vista
├── player.c         # Lógica de cada jugador (proceso independiente)
├── view.c           # Proceso de vista, imprime el estado del juego
├── shmlib.c/.h      # Biblioteca para manejo de memoria compartida y semáforos
├── Makefile         # Compila y ejecuta el proyecto
└── README.md

## Requisitos

- Sistema operativo compatible con POSIX (Linux)
- Compilador GCC
- Librerías:
  - pthread
  - rt
  - ncurses

## Compilación

Ejecutar: make

Esto compilará los binarios:
- master
- player
- view

Y la biblioteca shmlib.o.

## Ejecución del juego

El programa principal es master. Este lanza varios procesos player y un proceso view.

**Sintaxis:**

./master -w  -h  -d <delay_ms> -t <timeout_s> -s  -v ./view -p ./player [./player …]

**Ejemplo:**

./master -w 10 -h 10 -d 50 -t 10 -s 42 -v ./view -p ./player ./player ./player

- `-w`: ancho del tablero
- `-h`: alto del tablero
- `-d`: delay entre turnos en milisegundos
- `-t`: duración máxima del juego en segundos
- `-s`: semilla para el generador aleatorio
- `-v`: ruta al ejecutable de la vista
- `-p`: rutas a los ejecutables de los jugadores

## Descripción general

### master.c

- Inicializa la memoria compartida y los semáforos
- Lanza los procesos player y view usando fork + exec
- Coordina la ejecución de los turnos
- Procesa movimientos enviados por los jugadores a través de pipes
- Determina cuándo finaliza el juego (por tiempo o bloqueo total)
- Informa el resultado y muestra al jugador ganador (o empate)

### player.c

- Se conecta a la memoria compartida
- Busca su información dentro del estado global
- Solicita movimientos aleatorios
- Se bloquea si no puede realizar movimientos válidos

### view.c

- Se conecta a la memoria compartida en modo lectura
- Imprime en la terminal el tablero y la información de cada jugador
- Usa ncurses para una visualización en tiempo real
- Se actualiza solo cuando el master lo indica vía semáforos

### shmlib.c / shmlib.h

- Encapsula funciones para:
  - Crear y abrir memoria compartida
  - Mapear (mmap) y desmapear (munmap) regiones
  - Manejar semáforos POSIX (sem_t)
- Permite usar createSHM, openSHM, closeSHM de forma segura

## Testing con Valgrind

Podés ejecutar los procesos individualmente con valgrind:

valgrind ./player 10 10

O usar un wrapper script para que master ejecute los jugadores con valgrind.

## Limpieza

Elimina binarios y memoria compartida:

make clean

## Notas técnicas

- El tablero es una matriz lineal int board[], representada fila a fila
- Cada celda capturada por un jugador es representada como -idJugador
- Las celdas con 0 no se pueden capturar ni pisar
- Se utiliza sincronización de lectores-escritor para evitar race conditions
- Al finalizar, se determina el ganador por puntaje, movimientos válidos e inválidos

## Ejemplo de salida

Jugador: player
Puntaje: 124
Movs válidos: 21
Movs inválidos: 7
Posición: (3,5)

Tablero (10x10) | 2 jugadores
8   7   5   6   1  -1   3   2   9   4
7   8   0   9   4  -1   3   4   5   7
6   4   8   2   5   9   4   6   4   1
…

## Autor

Trabajo práctico realizado por Alexis Herrera Vegas, Sebastian Caules y Federico Kloberdanz para la materia 72.11 - Sistemas Operativo.

## Licencia

ITBA