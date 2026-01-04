CC = gcc
CFLAGS = -O -Wall
all : ssu_backup add recover help

ssu_backup : start.c
	$(CC) -o ssu_backup start.c -lcrypto

recover : recover.c
	$(CC) -o recover recover.c -lcrypto

add : add.c
	$(CC) -o add add.c -lcrypto

help : help.c
	$(CC) -o help help.c 

