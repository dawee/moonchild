sources := $(shell find . -name "*.cpp")
objects := $(subst .cpp,.o,${sources})
cflags := -DMOONCHILD_SIMULATOR

simulator: ${objects}
	@gcc ${cflags} -o simulator ${objects}

%.o: %.cpp
	@gcc ${cflags} -o $@ -c $<
