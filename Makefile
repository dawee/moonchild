fixtures := $(patsubst test/fixtures/%.lua,test/fixtures/build/%, $(wildcard test/fixtures/*.lua))
tests := $(patsubst test/test_%.c,test/run/%, $(wildcard test/*.c))

test: ${fixtures} ${tests}

clean:
	@rm -rf test/build test/fixtures/build

test/run/%: test/test_%.c
	@gcc -I./deps/greatest -o $@ $<
	@$@

test/fixtures/build/%: test/fixtures/%.lua
	@echo "> Build fixture" $<
	@./bin/moonchild build $<


.PHONY: test clean
