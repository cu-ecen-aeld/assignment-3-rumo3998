#Makefile to build the writer application

#This is conditional and depends on if the user passes CROSS_COMPILE 
#if CROSS_COMPILE is not used then the expression expands to gcc only
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror

#default target is the writer application
all: writer

writer: writer.c
	@echo "Running writer.c"
	$(CC) $(CFLAGS) writer.c -o writer
clean: 
	@echo "Running clean"
	rm -f writer writer.o *.out
