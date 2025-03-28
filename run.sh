#!/bin/bash

# Compilación
echo "🔨 Compilando archivos..."

gcc master.c -o master -lrt -lpthread || { echo "❌ Error compilando master.c"; exit 1; }
gcc player.c -o player -lrt -lpthread || { echo "❌ Error compilando player.c"; exit 1; }
gcc view.c -o view -lrt -lpthread || { echo "❌ Error compilando view.c"; exit 1; }

echo "✅ Compilación completa"

# Ejecutar master con argumentos
echo "🚀 Ejecutando el juego..."
./master -w 10 -h 10 -d 200 -t 10 -v ./view -p ./player ./player

# Limpieza
echo "🧹 Borrando binarios..."
rm -f master player view
echo "✅ Limpieza completa"