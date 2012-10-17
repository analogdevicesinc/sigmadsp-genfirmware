HELPERS = read_firmware gen_firmware

all: $(HELPERS)

clean:
	rm -f *.o $(HELPERS)

CFLAGS = -Wall -pedantic -std=c99 -O2 -g -D_BSD_SOURCE

gen_firmware: gen_firmware.c crc32.c
read_firmware: read_firmware.c crc32.c
