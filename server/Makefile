#Makefile to build the server application

#This is conditional and depends on if the user passes CROSS_COMPILE 
#if CROSS_COMPILE is not used then the expression expands to gcc only
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror

#default target is the writer application
all: aesdsocket 

aesdsocket: aesdsocket.c
	@echo "Running aesdsocket.c"
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket
clean: 
	@echo "Running clean"
	rm -f aesdsocket aesdsocket.o *.out
