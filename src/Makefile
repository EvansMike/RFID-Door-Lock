# Make file for Virgin Galactic Servo driver and serial control interface
# $Id: Makefile,v 1.6 2010/07/25 11:58:04 mikee Exp $
#
#
#


PROGRAM=open-sesame

MCU=atmega8
F_CPU = 1000000

FUSE_L  = 0x21
FUSE_H  = 0xD9

AVRDUDE = avrdude -c stk200 -p $(MCU)

# Place -D or -U options here
CDEFS = -DF_CPU=$(F_CPU) -DMCU=$(MCU)
CXXDEFS = -DF_CPU=$(F_CPU)

CC=avr-gcc
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump -D
CSRCS=$(PROGRAM).c serial.c wiegand.c
# TWI_Slave.c
COBJS := $(CSRCS:.c=.o)

HEADS := $(CSRCS:.c=.h)
AOBJS := $(COBJS:.asm=.o)
CLSTS := $(CSRCS:.c=.lst)

# The -u,vfprintf -lprintf_flt -lm are to anable sprintf and float point convertion libraries
CFLAGS=-g -mmcu=$(MCU) $(CDEFS) -Iusbdrv -Wl,-u,vfprintf -lprintf_flt -lm -I. -I/usr/avr/include/avr/ -lm -Wall -Wstrict-prototypes -Os -mcall-prologues -DDEBUG_LEVEL=0


all: $(PROGRAM).hex $(PROGRAM).eep libs

olist:
	@echo "$(COBJS)"

$(COBJS) : $(CSRCS) $(HEADS) Makefile
	$(CC) $(CFLAGS) -Os -c $(CSRCS)

# Link: create ELF output file from object files
$(PROGRAM).elf: $(COBJS)
	$(CC) $(CFLAGS) $^ -o $@


# Create hex output file from ELF output file.
$(PROGRAM).hex: $(PROGRAM).elf
	$(OBJCOPY) -R .eeprom -g -O ihex $< $@
	avr-size $(PROGRAM).hex

$(PROGRAM).eep: $(PROGRAM).elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $< $@
	avr-size $(PROGRAM).eep

# Target: clean project.
.PHONY: clean
clean:
	rm -f *.o *.elf *.hex *.map *.lst *.s *.eep

.PHONY: realclean
realclean: clean
	rm -f *~ *.bak

style:
	astyle --indent=spaces=4 --brackets=break --pad-oper --pad-header --suffix=none open-sesame.c

# erase the AVR before it is programmed
install: all
	$(AVRDUDE) -U flash:w:$(PROGRAM).hex:i
	$(AVRDUDE) -U eeprom:w:$(PROGRAM).eep:i
# rule for programming fuse bits:

fuse:
	@[ "$(FUSE_H)" != "" -a "$(FUSE_L)" != "" ] || { echo "*** Edit Makefile and choose values for FUSE_L and FUSE_H!"; exit 1; }
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m

libs: $(COBJS)
	avr-ar rcs libserial.a serial.o
	avr-ar rcs libwiegand.a wiegand.o


#-U hfuse:w:0xD9:m -U lfuse:w:0xe4:m
