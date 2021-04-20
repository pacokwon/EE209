CC = gcc

main: main.o sort.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c $<
