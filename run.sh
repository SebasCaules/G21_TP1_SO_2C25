#!/bin/bash

# Compilación

gcc master.c -o master -lm -lrt -lpthread || { echo "❌ Error compilando master.c"; exit 1; }
gcc player.c -o player -lrt -lpthread || { echo "❌ Error compilando player.c"; exit 1; }
gcc view.c -o view -lrt -lpthread || { echo "❌ Error compilando view.c"; exit 1; }

# Ejecutar master con argumentos
./master -w 10 -h 10 -d 200 -t 10 -s 90 -v ./view -p ./player ./player ./player ./player

# Limpieza
rm -f master player view
