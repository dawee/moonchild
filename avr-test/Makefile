
all: led.hex led.bin

re: clean all

clean:
	rm -f led.o led.bin led.elf led.hex

led.o: led.c
	avr-gcc -g -Os -mmcu=atmega16 -c led.c

led.elf: led.o
	avr-gcc -g -mmcu=atmega16 -o led.elf led.o

led.hex: led.elf
	avr-objcopy -j .text -j .data -O ihex led.elf led.hex

led.bin: led.elf
	avr-objcopy -j .text -j .data -O binary led.elf led.bin


.PHONY: clean all re

