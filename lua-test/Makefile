all: hello.c

hello.c: hello.lua
	LUA_PATH=../deps/lua2c/lib/?.lua lua ../deps/lua2c/lua2c.lua ../deps/lua2c/examples-lua/hello.lua > hello.c

.PHONY: all

