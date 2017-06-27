#makefile for project 1 works both in linux and unix system now
all : server
server : server.o
	gcc -o sws server.o 
server.o : server.c
	gcc -c server.c
clean :
	rm sws *.o
