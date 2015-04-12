
CFLAGS = -Wall -D_GNU_SOURCE
LIBS = -lpthread

all: streameye

streameye.o: streameye.c streameye.h client.h common.h
	gcc $(CFLAGS) -c -o streameye.o streameye.c

client.o: client.c client.h streameye.h common.h
	gcc $(CFLAGS) -c -o client.o client.c

streameye: streameye.o client.o
	gcc $(CFLAGS) -o streameye streameye.o client.o $(LIBS)

clean:
	rm -f *.o
	rm -f streameye
