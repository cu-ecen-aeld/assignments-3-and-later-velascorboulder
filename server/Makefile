CC = ${CROSS_COMPILE}gcc

default: all

all: aesdsocket

writer: aesdsocket.c
	${CC} $^ -o $@

.PHONY: clean
clean:
	rm -f aesdsocket *.o
