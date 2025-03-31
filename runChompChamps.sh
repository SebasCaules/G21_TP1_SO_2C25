#!/bin/bash

# Compilación de player y view
echo "🔨 Compilando player.c y view.c..."

# Compilación de biblioteca compartida
gcc -c shmlib.c -o shmlib.o || { echo "❌ Error compilando shmlib.c"; exit 1; }

gcc player.c shmlib.o -o player -lrt -lpthread || { echo "❌ Error compilando player.c"; exit 1; }
gcc view.c shmlib.o -o view -lrt -lpthread || { echo "❌ Error compilando view.c"; exit 1; }

echo "✅ Compilación completa"

# Ejecutar el binario oficial chompchamps
echo "🚀 Ejecutando ChompChamps..."
./ChompChamps -w 20 -h 20 -d 200 -t 10 -s 90 -v ./view -p ./player ./player ./player ./player ./player ./player ./player ./player ./player

# Limpieza
echo "🧹 Borrando binarios player y view..."
rm -f player view
echo "✅ Limpieza completa"