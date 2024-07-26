CC = ${CROSS_COMPILE}gcc

default: all

all: aesdsocket

aesdsocket: aesdsocket.c
	${CC} $^ -o $@ -lpthread

.PHONY: clean
clean:
	rm -f aesdsocket *.o
