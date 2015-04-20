
CFLAGS = -Wall -O2 -D_GNU_SOURCE
#CFLAGS = -Wall -g -D_GNU_SOURCE
LDFLAGS = -lpthread

all: streameye

streameye.o: streameye.c streameye.h client.h common.h
	$(CC) $(CFLAGS) -c -o streameye.o streameye.c

client.o: client.c client.h streameye.h common.h
	$(CC) $(CFLAGS) -c -o client.o client.c

streameye: streameye.o client.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o streameye streameye.o client.o

clean:
	rm -f *.o
	rm -f streameye

