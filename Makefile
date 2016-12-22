sources := $(shell find src -name "*.c")
cflags := -I./src -Wall -Werror

test: clean-test avr-build-test fixtures header-test

fixtures:
	@luac -o test/fixtures/empty.out test/fixtures/empty.lua

avr-build-test:
	@avr-gcc -g -Os -mmcu=atmega32 ${cflags} -c ${sources}

clean-test:
	@rm -f test/*-test

%-test:
	@gcc ${cflags} ${sources} test/$@.c -o test/$@
	@./test/$@


.PHONY: test