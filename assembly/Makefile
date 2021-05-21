AS = as
CC = gcc

dc:

run:
	docker compose run -d --name asm asm

down:
	docker compose down

exec:
	docker exec -it asm /bin/bash

%: src/%.s
	$(AS) --32 -march=i386 $< -o $*.o
	$(CC) -m32 -march=i386 $*.o -o $*

.PHONY: run down exec
