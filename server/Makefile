#Makefile to build the server application

#This is conditional and depends on if the user passes CROSS_COMPILE 
#if CROSS_COMPILE is not used then the expression expands to gcc only
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -I.

#default and all targets for the server application

all: default 
default: aesdsocket 

aesdsocket: aesdsocket.c aesdsocket.h
	@echo "Running aesdsocket.c"
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:	
	@echo "Running clean"
	rm -f aesdsocket aesdsocket.o 
