fixtures := $(patsubst test/fixtures/%.lua,test/fixtures/build/%, $(wildcard test/fixtures/*.lua))


test: fixtures

fixtures: fixtures-pre-build ${fixtures}

fixtures-pre-build:
	@mkdir -p test/fixtures/build

test/fixtures/build/%: test/fixtures/%.lua
	@echo "Build fixture" $<
	@./bin/moonchild build $<


.PHONY: test
