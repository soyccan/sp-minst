CC=gcc
CFLAGS=-Wall -Wextra -Wconversion -std=c18 -lm -lpthread
GPROF_FLAG=-pg
SRC=hw4.c

all: release

release: $(SRC)
	$(CC) $(CFLAGS) $(GPROF_FLAG) -D NDEBUG -O3 -o hw4 hw4.c

debug: $(SRC)
	$(CC) $(CFLAGS) $(GPROF_FLAG) -D DEBUG -g -o hw4 hw4.c
