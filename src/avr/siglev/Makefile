# Tell both the compiler and the programmer application (avrdude)
# what MCU we are using
MCU=atmega8
PART=m8

# Tell the programmer application (avrdude) what hardware we use
PROG_HW=avrusb500

# Name of application
NAME:=siglev

# Source files to compile
SRCS:=siglev.c

# Use the AVR gcc compiler and tools
CC=avr-gcc
OBJCOPY=avr-objcopy

# What CFLAGS to use
CFLAGS=-g -mmcu=$(MCU) -Wall -Wstrict-prototypes -O2 -mcall-prologues

# Setup the object and dependency file lists
OBJS:=$(SRCS:.c=.o)
DEP_FILES:=$(SRCS:.c=.d)


# Generic targets. You should not need to touch these
all: $(NAME).hex

$(NAME).hex:	$(NAME).out
	$(OBJCOPY) -R .eeprom -O ihex $< $@

$(NAME).out:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(NAME).map $(OBJS)
	
%.o:	%.c
	$(CC) $(CFLAGS) -c $<

%.d:	%.c
	$(CC) -M -MG -MT "$*.o $@" $(CFLAGS) -MF $@ $< > /dev/null


# Load program into flash
.PHONY: load
load:
	avrdude -p $(PART) -c $(PROG_HW) -e -y -U flash:w:$(NAME).hex


# Setup external crystal
.PHONY: fuse
fuse:
	avrdude -p $(PART) -c $(PROG_HW) -U hfuse:w:0xc9:m
	avrdude -p $(PART) -c $(PROG_HW) -U lfuse:w:0xef:m


# Clean up source directory
.PHONY: clean
clean:
	rm -f $(OBJS) $(DEP_FILES) $(NAME).map $(NAME).out $(NAME).hex *~ sin_data.h


# Include dependency files
-include $(DEP_FILES)


# Extra user defined targets below
sin_data.h: sin_gen.rb
	ruby sin_gen.rb > $@

