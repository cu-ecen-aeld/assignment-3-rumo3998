#Makefile to build the server application

#build switch for the USE_AESD_CHAR_DEVICE
USE_AESD_CHAR_DEVICE ?= 1

#This is conditional and depends on if the user passes CROSS_COMPILE 
#if CROSS_COMPILE is not used then the expression expands to gcc only
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -I.
LDFLAGS ?= -lpthread -lrt

ifeq ($(USE_AESD_CHAR_DEVICE),1)
       CFLAGS += -DUSE_AESD_CHAR_DEVICE=1
endif       

#default and all targets for the server application
TARGET ?= aesdsocket

all: $(TARGET) 

$(TARGET): aesdsocket.c aesdsocket.h
	@echo "Running compilation with $(CC)..."
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:	
	@echo "Running clean"
	rm -f $(TARGET) *.o 
