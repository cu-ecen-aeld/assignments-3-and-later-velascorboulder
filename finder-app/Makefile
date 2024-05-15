CC = ${CROSS_COMPILE}gcc

writer: writer.c
	${CC} $^ -o $@

.PHONY: clean
clean:
	rm -f writer *.o
