#!/bin/bash

# Compilación de player y view
echo "🔨 Compilando player.c y view.c..."

gcc player.c -o player -lrt -lpthread || { echo "❌ Error compilando player.c"; exit 1; }
gcc view.c -o view -lrt -lpthread || { echo "❌ Error compilando view.c"; exit 1; }

echo "✅ Compilación completa"

# Ejecutar el binario oficial chompchamps
echo "🚀 Ejecutando ChompChamps..."
./ChompChamps -w 10 -h 10 -d 200 -t 10 -v ./view -p ./player ./player

# Limpieza
echo "🧹 Borrando binarios player y view..."
rm -f player view
echo "✅ Limpieza completa"