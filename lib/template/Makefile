include config.mk

HOST_TARGET ?= target/host/${project_basename}
PROJECT_MAIN ?=
EXTRAS_FLAGS ?=

moon_src := $(wildcard ${moon_root}/src/moonchild/*.c)
simulator_src := $(wildcard ${moon_root}/src/simulator/**/*.c)

host_objects := $(patsubst ${moon_root}/src/%.c, target/host/%.o, ${moon_src} ${simulator_src})
avr_objects := $(patsubst ${moon_root}/src/%.c, target/avr/%.o, ${moon_src})

ifeq ($(strip $(PROJECT_MAIN)),)
	common_flags := -I${moon_root}/src/moonchild ${EXTRAS_FLAGS}
else
	common_flags := -I${moon_root}/src/moonchild ${EXTRAS_FLAGS} -DMOONCHILD_PROJECT_MAIN=${PROJECT_MAIN}
endif

avr_flags := ${common_flags} -DF_CPU=${avr_fcpu}
host_flags := ${common_flags} \
	-I${moon_root}/src/simulator \
	-DMOONCHILD_SIMULATOR

deploy: avr
	@avrdude -F -V -c arduino -p ${avr_mmcu} -P ${avr_port} -b ${avr_baudrate} -U flash:w:target/avr/${project_basename}.hex

host: ${host_objects}
	@${CC} ${host_flags} -o ${HOST_TARGET} ${host_objects} ${project_basename}.c ${PROJECT_MAIN}

target/host/%.o: ${moon_root}/src/%.c
	@mkdir -p $(dir $@)
	@${CC} ${host_flags} -o $@ -c $<

avr: target/avr/${project_basename}.hex

target/avr/${project_basename}.hex: target/avr/${project_basename}.bin
	@${AVR_OBJCOPY} -O ihex -R .eeprom target/avr/${project_basename}.bin target/avr/${project_basename}.hex

target/avr/${project_basename}.bin: ${avr_objects}
	@${AVR_GCC} ${avr_flags} -mmcu=${avr_mmcu} -o target/avr/${project_basename}.bin ${avr_objects} ${project_basename}.c ${PROJECT_MAIN}

target/avr/%.o: ${moon_root}/src/%.c
	@mkdir -p $(dir $@)
	@${AVR_GCC} ${avr_flags} -Os -mmcu=${avr_mmcu} -c -o $@ $<
