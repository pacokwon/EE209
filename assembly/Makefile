AS = as
CC = gcc
CFLAGS = -Wall -Werror -ansi -pedantic -std=c99

mydc:

run:
	docker compose run -d --name asm asm

down:
	docker compose down

exec:
	docker exec -it asm /bin/bash

archive: submission.tar.gz

submission.tar.gz: mydc.s tests Makefile readme
	tar -cvzf $@ $^

%: %.s
	$(AS) --32 -march=i386 $< -o $*.o
	$(CC) $(CFLAGS) -m32 -march=i386 $*.o -o $*

test: mydc
	make -C tests all

clean:
	make -C tests clean
	@rm mydc mydc.o

.PHONY: run down exec clean
