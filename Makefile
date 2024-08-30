
CC=gcc
CFLAGS= -I backoffAlgorithm/source/include

all: 
	$(CC) $(CFLAGS) main.c backoffAlgorithm/source/backoff_algorithm.c -o main -lmosquitto

clean:
	rm main