CC = gcc
CFLAGS = -Iinclude

run%: build/client%
	./$< $(ARGS)

clean:
	rm -rf build

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/client%: build/testclient.o build/customer_manager%.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: run% clean
