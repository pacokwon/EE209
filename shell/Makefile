CC = gcc
CFLAGS = -g3 -Iinclude -Wall -Werror -ansi -std=c99 -D_DEFAULT_SOURCE -D_GNU_SOURCE
LDFLAGS = -Wall -Werror -std=c99

ish: build/ish.o build/parse.o build/dynarray.o
	$(CC) $(LDFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf build
