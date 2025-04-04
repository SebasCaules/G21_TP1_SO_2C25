CC = gcc
CFLAGS = -Wall -std=c99
LDFLAGS = -lm -lrt -lpthread

BINARIES = master player view
SHM_FILES = /dev/shm/game_state /dev/shm/game_sync

all: clean_shm $(BINARIES) run clean_bin

master: master.c shmlib.o
	$(CC) $(CFLAGS) -o master master.c shmlib.o $(LDFLAGS)

player: player.c shmlib.o
	$(CC) $(CFLAGS) -o player player.c shmlib.o $(LDFLAGS)

view: view.c shmlib.o
	$(CC) $(CFLAGS) -o view view.c shmlib.o $(LDFLAGS)

shmlib.o: shmlib.c shmlib.h
	$(CC) $(CFLAGS) -c shmlib.c -o shmlib.o

run:
	./master -w 10 -h 10 -d 50 -t 10 -s 90 -v ./view -p ./player ./player ./player

clean_shm:
	@if [ -e /dev/shm/game_state ]; then rm /dev/shm/game_state; fi
	@if [ -e /dev/shm/game_sync ]; then rm /dev/shm/game_sync; fi

clean_bin:
	@for f in $(BINARIES) shmlib.o; do \
		if [ -f $$f ]; then rm $$f; fi \
	done