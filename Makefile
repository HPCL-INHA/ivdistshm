CC=gcc
CFLAGS=-O3 -Wall -fpic

all: ivdistshm.o stub.o
	$(CC) $(CFLAGS) -shared -o ivdistshm.so ivdistshm.o stub.o

ivdistshm.o:
	$(CC) $(CFLAGS) -c -o ivdistshm.o src/ivdistshm.c

stub.o:
	$(CC) $(CFLAGS) -c -o stub.o src/stub.c

clean:
	rm -f *.o
