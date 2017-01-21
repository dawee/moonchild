module.exports = project => `
cflags := -DMOONCHILD_SIMULATOR -DMOONCHILD_DEBUG -I${project.moonRoot}/src/moonchild -I${project.moonRoot}/src/sim -I${project.moonRoot}/deps/uthash/src

simulator: \${objects}
\t@gcc \${cflags} -o simulator ${project.moonRoot}/src/sim/avr/pgmspace.c ${project.moonRoot}/src/moonchild/monitor.c ${project.moonRoot}/src/moonchild/moonchild.c *.c

%.o: %.cpp
\t@gcc \${cflags} -o $@ -c $<
`
