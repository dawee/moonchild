module.exports = project => `
#
# Makefile for '${project.fileName}' build
#

moon_root := ${project.moonRoot}
project_basename = ${project.baseName}
avr_fcpu := 16000000UL
avr_mmcu := atmega328p
avr_port := /dev/ttyUSB0
avr_baudrate := 57600

AVR_GCC := avr-gcc
AVRDUDE := avrdude
AVR_OBJCOPY := avr-objcopy
`
