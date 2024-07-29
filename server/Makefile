CC ?= ${CROSS_COMPILE}gcc
CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt

default: all

all: aesdsocket

aesdsocket: aesdsocket.c
	${CC} $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f aesdsocket *.o
