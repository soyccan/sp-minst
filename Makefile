CC=gcc
CFLAGS=-Wall -Wextra -Wconversion -std=c18 -lm -lpthread
SRC=hw4.c

all: release

release: $(SRC)
	$(CC) $(CFLAGS) -D NDEBUG -O3 -o hw4 hw4.c

debug: $(SRC)
	$(CC) $(CFLAGS) -D DEBUG -g -o hw4 hw4.c
