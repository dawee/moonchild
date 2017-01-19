module.exports = project => `
cflags := -DMOONCHILD_SIMULATOR -DMOONCHILD_DEBUG -I${project.moonRoot}/src/sim -I${project.moonRoot}/deps/uthash/src

simulator: \${objects}
\t@gcc \${cflags} -o simulator ${project.moonRoot}/src/sim/avr/pgmspace.cpp ${project.moonRoot}/src/sim/Arduboy.cpp *.cpp

%.o: %.cpp
\t@gcc \${cflags} -o $@ -c $<
`
