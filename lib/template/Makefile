include config.mk

moon_src := $(wildcard ${moon_root}/src/moonchild/*.c)
simulator_src := $(wildcard ${moon_root}/src/simulator/**/*.c)

host_objects := $(patsubst ${moon_root}/src/%.c, target/host/%.o, ${moon_src} ${simulator_src})

common_flags := -I${moon_root}/src/moonchild
host_flags := ${common_flags} \
	-I${moon_root}/src/simulator \
	-DMOONCHILD_SIMULATOR

host: ${host_objects}
	@${CC} ${host_flags} -o target/host/${project_basename} ${host_objects} ${project_basename}.c

target/host/%.o: ${moon_root}/src/%.c
	@mkdir -p $(dir $@)
	@${CC} ${host_flags} -o $@ -c $<
