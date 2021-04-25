CC = gcc
CFLAGS = -Iinclude

run%: build/client%
	./$< $(ARGS)

clean:
	rm -rf build

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/client%: src/testclient.o src/customer_manager%.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: run% clean
