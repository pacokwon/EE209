.SECONDARY:

MY_DC = @../dc
DC = @dc
RM = @rm
TXT_FILES = $(shell find . -type f -name '*.txt')
RESULT_FILES = $(TXT_FILES:.txt=.result)
EXPECTED_FILES = $(TXT_FILES:.txt=.expected)

all: $(RESULT_FILES)

clean:
	$(RM) -f $(RESULT_FILES) $(EXPECTED_FILES)

%.result: %.output %.expected
	@echo -n $*' '
	@if cmp --silent -- $*.output $*.expected; then \
		echo "PASS" | tee $*.result; \
	else \
		echo "FAIL" | tee $*.result; \
	fi;

%.output: ../dc %.txt
	$(MY_DC) < $*.txt > $*.output

%.expected: %.txt
	$(DC) < $*.txt > $*.expected 2>&1 || true

.PHONY: all clean
