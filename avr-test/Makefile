all: led.hex

led.o: led.c
	avr-gcc -g -Os -mmcu=atmega32 -c led.c

led.elf: led.o
	avr-gcc -g -mmcu=atmega32 -o led.elf led.o

led.hex: led.elf
	avr-objcopy -j .text -j .data -O ihex led.elf led.hex


.PHONY: all

