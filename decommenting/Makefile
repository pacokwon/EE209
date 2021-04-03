CC = gcc
CFLAGS = -Wall -Werror -ansi -pedantic -std=c99
TESTS = test0 test1 test2 test3 test4 test5 test6 test7 test8 test9

decomment: decomment.c
	$(CC) $(CFLAGS) -o decomment decomment.c

results/%.output results/%.error: tests/%.c decomment
	@mkdir -p results
	-./decomment < $< > results/$*.output 2> results/$*.error

sample/%.output sample/%.error: tests/%.c sampledecomment
	@mkdir -p sample
	-./sampledecomment < $< > sample/$*.output 2> sample/$*.error

results/%.result: results/%.output results/%.error sample/%.output sample/%.error
	@echo "------- Output Comparison -------" | tee $@
	@diff results/$*.output sample/$*.output | tee -a $@
	@echo "---------------------------------" | tee -a $@
	@echo "------- Error  Comparison -------" | tee -a $@
	@diff results/$*.error sample/$*.error | tee -a $@
	@echo "---------------------------------" | tee -a $@

clean:
	rm -rf sample results

.PRECIOUS: results/%.output results/%.error sample/%.output sample/%.error
.PHONY: clean
