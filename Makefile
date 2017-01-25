tests := $(patsubst %.c, %.run, $(wildcard test/**/*.c))

test: clean ${tests}

clean:
	@rm -rf $(wildcard test/**/*.run) $(wildcard test/**/build)

test/%.run: test/%.c
	@./bin/moonchild build -s $(join $(dir $@), fixture.lua)
	@HOST_TARGET=$(abspath $@) \
	 PROJECT_MAIN=$(abspath $<) \
	 EXTRAS_FLAGS=-I$(abspath ./deps/greatest) \
	 make -sC $(join $(dir $@), build) host
	@$@

.PHONY: clean test
