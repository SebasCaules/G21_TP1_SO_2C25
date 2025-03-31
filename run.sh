#!/bin/bash

# Compilación de biblioteca compartida
gcc -c shmlib.c -o shmlib.o || { echo "❌ Error compilando shmlib.c"; exit 1; }

# Compilación de cada ejecutable con shmlib.o incluido
gcc master.c shmlib.o -o master -lm -lrt -lpthread || { echo "❌ Error compilando master.c"; exit 1; }
gcc player.c shmlib.o -o player -lrt -lpthread || { echo "❌ Error compilando player.c"; exit 1; }
gcc view.c shmlib.o -o view -lrt -lpthread || { echo "❌ Error compilando view.c"; exit 1; }

# Ejecutar master con argumentos
./master -w 20 -h 20 -d 200 -t 10 -s 90 -v ./view -p ./player ./player ./player ./player ./player ./player ./player ./player ./player

# Limpieza
rm -f master player view shmlib.o
