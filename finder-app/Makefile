CC = ${CROSS_COMPILE}gcc

all: writer

writer: writer.c
	${CC} $^ -o $@

.PHONY: clean
clean:
	rm -f writer *.o
