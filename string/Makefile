CC = gcc
CFLAGS = -Iinclude
TESTS = strlen strcpy strcmp strchr strstr strcat strtol
CLIENT_TESTS = StrGetLength StrCopy StrCompare StrFindStr StrConcat StrFindChr StrToLong

sgrep: build/sgrep

archive:
	tar -cvzf submission.tar.gz readme -C src sgrep.c str.c -C ../include str.h

# run custom test
str-custom:
	@for test in $(TESTS); do \
		$(MAKE) $$test.test; \
	done

# run provided test
str-provided: build/tests/client
	@for test in $(CLIENT_TESTS); do \
		./build/tests/client $$test; \
	done

clean:
	rm -rf build

build/str.o: src/str.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/tests/%.o: tests/%.c
	@mkdir -p build/tests
	$(CC) $(CFLAGS) -c $< -o $@

build/tests/%.bin: build/tests/%.o build/str.o
	$(CC) $(CFLAGS) $^ -o build/tests/$*.bin

build/tests/client: src/client.c
	@mkdir -p build/tests
	$(CC) $(CFLAGS) src/str.c src/client.c -o build/tests/client

build/sgrep: src/sgrep.c
	$(CC) $(CFLAGS) src/str.c src/sgrep.c -o build/sgrep

%.test: build/tests/%.bin
	@if [ -e ./build/tests/$*.bin ]; then \
		./build/tests/$*.bin; \
	else \
		@echo "file does not exist!"; \
	fi

.PHONY: archive str-custom str-provided %.test clean sgrep
.PRECIOUS: build/str.o build/tests/%.o build/tests/%.bin
