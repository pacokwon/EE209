CC = gcc
CFLAGS = -Iinclude
TESTS = strlen strcpy strcmp strchr strstr strcat strtol

all:
	@for test in $(TESTS); do \
		$(MAKE) $$test.result; \
	done

clean:
	rm -rf build

build/str.o: src/str.c include/str.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/tests/%.o: tests/%.c
	@mkdir -p build/tests
	$(CC) $(CFLAGS) -c $< -o $@

%.result: build/tests/%.bin
	@if [ -e ./build/tests/$*.bin ]; then \
		./build/tests/$*.bin; \
	else \
		@echo "file does not exist!"; \
	fi

build/tests/%.bin: build/tests/%.o build/str.o
	$(CC) $(CFLAGS) $? -o build/tests/$*.bin

.PHONY: %.result clean
.PRECIOUS: build/str.o build/tests/%.o build/tests/%.bin
