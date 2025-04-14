CC = gcc
CFLAGS = -Wall -std=c99 -D_XOPEN_SOURCE=500
LDFLAGS = -lm -lrt -lpthread
VALFLAGS = --leak-check=full --track-origins=yes

BINARIES = master player view
SHM_FILES = /dev/shm/game_state /dev/shm/game_sync

RUN_CMD = ./master -w 10 -h 10 -d 50 -t 10 -s 90 -v ./view -p ./player ./player ./player

all: compile run clean

compile: clean_shm $(BINARIES)

clean: clean_shm clean_bin clean_pvs

master: master.c shmlib.o
	$(CC) $(CFLAGS) -o master master.c shmlib.o $(LDFLAGS)

player: player.c shmlib.o
	$(CC) $(CFLAGS) -o player player.c shmlib.o $(LDFLAGS)

view: view.c shmlib.o
	$(CC) $(CFLAGS) -o view view.c shmlib.o $(LDFLAGS)

shmlib.o: shmlib.c shmlib.h
	$(CC) $(CFLAGS) -c shmlib.c -o shmlib.o

run:
	$(RUN_CMD)

clean_shm:
	@if [ -e /dev/shm/game_state ]; then rm /dev/shm/game_state; fi
	@if [ -e /dev/shm/game_sync ]; then rm /dev/shm/game_sync; fi

clean_bin:
	@for f in $(BINARIES) shmlib.o; do \
		if [ -f $$f ]; then rm $$f; fi \
	done

clean_pvs:
	@if [ -e PVS-Studio.log ]; then rm PVS-Studio.log; fi
	@if [ -e report.tasks ]; then rm report.tasks; fi
	@if [ -e strace_out ]; then rm strace_out; fi

valgrind-check: compile
	valgrind $(VALFLAGS) $(RUN_CMD)
	clean

pvs-check:
	pvs-studio-analyzer trace -- make compile
	pvs-studio-analyzer analyze -o PVS-Studio.log
	plog-converter -a GA:1,2 -t tasklist -o report.tasks PVS-Studio.log