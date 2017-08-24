all: transmitter

transmitter.hex:
	avr-gcc -DF_CPU=16000000 -mmcu=atmega328p -Wall -Os -o transmitter.elf RCSwitch.h RCSwitch.c main_transmitter.c
	avr-objcopy -j .text -j .data -O ihex transmitter.elf transmitter.hex

receiver.hex:
	avr-gcc -DF_CPU=8000000 -mmcu=atmega328p -Wall -Os -o receiver.elf RCSwitch.h RCSwitch.c main_receiver.c
	avr-objcopy -j .text -j .data -O ihex receiver.elf receiver.hex

transmitter: transmitter.hex
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:transmitter.hex

receiver: receiver.hex
	avrdude -c usbasp -p atmega328p -U flash:w:receiver.hex

clean:
	rm -f *.hex *.elf
