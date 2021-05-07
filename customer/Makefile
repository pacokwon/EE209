CC = gcc209
CFLAGS = -Iinclude -D_GNU_SOURCE

archive: src/customer_manager1.c src/customer_manager1.c readme
	tar -cvzf submission.tar.gz readme -C src customer_manager1.c customer_manager2.c

run%: build/client%
	./$< $(ARGS)

clean:
	rm -rf build

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/client%: build/testclient.o build/customer_manager%.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: run% clean archive
