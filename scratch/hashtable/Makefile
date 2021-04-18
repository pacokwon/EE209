CC = gcc

main: main.o hashtable.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c $<
