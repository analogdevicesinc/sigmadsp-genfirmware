/**
 * Copyright (C) 2009-2012 Analog Devices, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *
 * Written by Mike Frysinger <vapier@gentoo.org>
 *
 * Sigma Firmware debug helper: dump a sigma firmware binary blob
 * into some human readable strings.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <error.h>
#include <errno.h>

#include "gen_firmware.h"

#define SIZEOF_sf (7 + 1 + 4)
#define LEN_sa(sa) ((uint32_t)(((sa)->len_hi << 16) | le16toh((sa)->len)))
#define LEN_sa_payload(sa) (LEN_sa(sa) ? LEN_sa(sa) - 2 : 0)
#define SIZEOF_sa(sa) (1 + 1 + 2 + 2 + 2 + LEN_sa_payload(sa) + (LEN_sa_payload(sa) % 2))

static void dump(const unsigned char *buf, int x)
{
	while (x--)
		printf("'%c'%s ", *buf++, x ? "," : "");
}

static void dumpx(const unsigned char *buf, int x)
{
	while (x--)
		printf("0x%02x%s ", *buf++, x ? "," : "");
}

static const char *decode_inst(unsigned int inst)
{
	switch (inst) {
		case 0: return "writexbytes";
		case 1: return "writesingle";
		case 2: return "writesafeload";
		case 3: return "delay";
		case 4: return "pllwait";
		case 5: return "noop";
		case 6: return "end";
		default: return "???";
	}
}

static char buf[8096];

int main(int argc, char *argv[])
{
	int i, j;
	FILE *fp;
	const char *file;
	struct sigma_firmware_header *sf;
	struct sigma_action *sa;
	size_t bytes;

	memset(buf, 0xad, sizeof(buf));

	printf("sizeof(sf) = %zu vs SIZEOF_sf = %i\n", sizeof(*sf), SIZEOF_sf);
	printf("sizeof(sa) = %zu\n", sizeof(*sa));

	for (i = 1; i < argc; ++i) {
		file = argv[i];
		printf("firmware blob decode: %s\n", file);
		fp = fopen(file, "r");
		if (!fp) {
			error(0, errno, "Failed to open \"%s\"", file);
			continue;
		}
		bytes = fread(buf, 1, sizeof(buf), fp);

		sf = (void *)buf;

		printf(
			"struct sigma_firmware sf {\n"
			"	unsigned char magic[7] = { "
		);
		dump(sf->magic, 7);
		printf("};\n");
		printf(
			"	uint8_t version = %i;\n"
			"	uint32_t crc = 0x%08x;\n"
			"};\n",
			sf->version, le32toh(sf->crc));

		j = 0;
		sa = (void *)(buf + SIZEOF_sf);
		bytes -= SIZEOF_sf;
		while (bytes > 0) {
			size_t new_bytes;
			while (bytes < SIZEOF_sa(sa)) {
				new_bytes = fread(buf + bytes, 1, sizeof(buf) - bytes, fp);
				if (new_bytes == 0)
					break;
				bytes += new_bytes;
			}
			if (bytes < SIZEOF_sa(sa)) {
				printf("Corrupt payload: %zd %d\n", bytes, SIZEOF_sa(sa));
				break;
			}
			bytes -= SIZEOF_sa(sa);

			printf(
				"struct sigma_action sa%i = {\n"
				"	uint8_t instr  = %#x /* %u (%s) */;\n"
				"	uint8_t len_hi = %#x /* %u */;\n"
				"	uint16_t len   = %#x /* %u */;\n"
				"	uint16_t addr  = %#x /* %u */;\n",
				j,
				sa->instr, sa->instr, decode_inst(sa->instr),
				sa->len_hi, sa->len_hi,
				le16toh(sa->len), le16toh(sa->len),
				be16toh(sa->addr), be16toh(sa->addr));

			if (sa->instr == 3)
				sa->len = 0;

			printf("	unsigned char payload[%u]", LEN_sa_payload(sa));
			if (LEN_sa_payload(sa)) {
				printf(" = {\n\t\t");
				dumpx(sa->payload, LEN_sa_payload(sa));
				printf("\n");
				printf("	};\n");
			} else
				printf(";\n");
			printf("};\n");

			sa = (struct sigma_action *)((uint8_t *)sa + SIZEOF_sa(sa));
			++j;
		}
	}

	return 0;
}
