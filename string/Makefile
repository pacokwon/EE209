CC = gcc
CFLAGS = -Iinclude
TESTS = strlen strcpy strcmp strchr strstr strcat strtol

all:
	@for test in $(TESTS); do \
		$(MAKE) $$test.test; \
	done

clean:
	rm -rf build

build/str.o: src/str.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/tests/%.o: tests/%.c
	@mkdir -p build/tests
	$(CC) $(CFLAGS) -c $< -o $@

%.test: build/tests/%.bin
	@if [ -e ./build/tests/$*.bin ]; then \
		./build/tests/$*.bin; \
	else \
		@echo "file does not exist!"; \
	fi

build/tests/%.bin: build/tests/%.o build/str.o
	$(CC) $(CFLAGS) $^ -o build/tests/$*.bin

.PHONY: %.test clean
.PRECIOUS: build/str.o build/tests/%.o build/tests/%.bin
