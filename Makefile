sources := $(shell find src -name "*.c")
fixtures := $(shell find test/fixtures -name "*.lua")
fixtures_out := $(subst .lua,.out,${fixtures})
cflags := -I./src -Wall -Werror

test: clean-test avr-build-test ${fixtures_out} header-test

test/fixtures/%.out: test/fixtures/%.lua
	@./deps/lua-5.3.3/src/luac -o $@ $<

avr-build-test:
	@avr-gcc -g -Os -mmcu=atmega32 ${cflags} -c ${sources}

clean-test:
	@rm -f test/*-test

%-test:
	@gcc ${cflags} ${sources} test/$@.c -o test/$@
	@./test/$@


.PHONY: test