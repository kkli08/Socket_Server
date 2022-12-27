all: client server groff

server: server.o tands.o
	gcc -O -pthread server.o tands.o -o server

client: client.o tands.o
	gcc -O client.o tands.o -o client

server.o: server.c
	gcc -O -pthread -c -o server.o server.c

client.o: client.c
	gcc -O -c -o client.o client.c

tands.o: tands.c
	gcc -c -o tands.o tands.c

groff:
	groff -ms manual.ms -T pdf > manual.pdf

clean:
	rm -rf *.o *.log *.pdf client server