CC = gcc

main: main.o list.o
	$(CC) -o $@ $^

list.o: list.c
	$(CC) -c $<

main.o: main.c
	$(CC) -c $<
