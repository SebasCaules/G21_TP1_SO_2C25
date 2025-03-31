#!/bin/bash

# Limpieza inicial de binarios (solo si existen)
[ -f master ] && rm master
[ -f player ] && rm player
[ -f view ] && rm view
[ -f shmlib.o ] && rm shmlib.o

# Eliminar SHM previas si existen
[ -e /dev/shm/game_state ] && rm /dev/shm/game_state
[ -e /dev/shm/game_sync ] && rm /dev/shm/game_sync

# Pedir cantidad de jugadores
read -p "Ingrese la cantidad de jugadores (1-9): " num_players
if ! [[ "$num_players" =~ ^[1-9]$ ]]; then
    echo "Número inválido. Debe ser un entero entre 1 y 9."
    exit 1
fi

if (( num_players > 9 )); then
    echo "Maximum number of players is 9."
    exit 1
fi

# Compilación
gcc -c shmlib.c -o shmlib.o || { echo "Error compilando shmlib.c"; exit 1; }
gcc master.c shmlib.o -o master -Wall -lm -lrt -lpthread || { echo "Error compilando master.c"; exit 1; }
gcc player.c shmlib.o -o player -Wall -lrt -lpthread || { echo "Error compilando player.c"; exit 1; }
gcc view.c shmlib.o -o view -Wall -lrt -lpthread || { echo "Error compilando view.c"; exit 1; }

# Construir lista de jugadores (exactamente la cantidad solicitada)
player_args=""
for ((i=0; i<num_players-1; i++)); do
    player_args+=" ./player"
done

# Ejecutar master con vista y jugadores
eval ./master -w 10 -h 10 -d 50 -t 10 -s 90 -v ./view -p ./player$player_args

# Limpieza final de binarios
[ -f master ] && rm master
[ -f player ] && rm player
[ -f view ] && rm view
[ -f shmlib.o ] && rm shmlib.o