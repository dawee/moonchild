tests := $(wildcard test/**/*.c)

test:
	@echo ${tests}

test/%.run: test/%.c
	@./bin/moonchild build -s $(join $(dir $@), fixture.lua)

.PHONY: test
