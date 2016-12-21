sources := $(shell find src -name "*.c")
cflags := -I./src

test: clean-test avr-build-test fixtures header-test

fixtures:
	@luac -o test/fixtures/empty.out test/fixtures/empty.lua

avr-build-test:
	@avr-gcc -g -Os -mmcu=atmega32 ${cflags} -c ${sources}

clean-test:
	@rm -f test/*-test

header-test:
	@gcc ${cflags} ${sources} test/header-test.c -o test/header-test
	@./test/header-test


.PHONY: test