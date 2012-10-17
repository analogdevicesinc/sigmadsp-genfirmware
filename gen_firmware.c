/**
 * Copyright (C) 2009-2012 Analog Devices, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *
 * Usage:
 *   genfirmware [<parameter.bin> <program.bin> [<firmware.bin>]]
 **/
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <error.h>
#include <errno.h>

#include "gen_firmware.h"
#include "crc32.h"

static uint8_t sigma_prog[MAX_LEN];
static uint8_t sigma_param[MAX_LEN];
static size_t sigma_program_size;
static size_t sigma_param_size;

static int read_file(const char *filename, uint8_t *buf, size_t *size)
{
	int fd;
	ssize_t ret;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		error(0, errno, "Failed to open \"%s\"", filename);
		return -1;
	}
	ret = read(fd, buf, MAX_LEN);
	close(fd);

	if (ret < 0)
		return -1;

	*size = ret;

	return 0;
}

#define ROUND_UP(x, y) ((x) + ((x) % (y)))

static struct sigma_action *sigma_action_alloc(size_t payload_size,
		uint8_t *payload, size_t *action_size)
{
	struct sigma_action *action;

	*action_size = sizeof(*action) + ROUND_UP(payload_size, sizeof(uint16_t));

	action = malloc(*action_size);
	if (!action)
		return NULL;
	action->len = htole16(payload_size & 0xffff);
	action->len_hi = htole16((payload_size >> 16) & 0xff);
	action->instr = SIGMA_ACTION_WRITEXBYTES;
	/* I2C transfer starts from MSB */
	action->addr = htobe16((payload[0] << 8) | payload[1]);
	memcpy(action->payload, payload + 2, payload_size - 2);

	return action;
}

/**
 * Got the parameter.bin and program.bin by "Save as Raw Data"->
 * "Adress+Data" in SigmaStudio.
 **/
int main(int argc, char **argv)
{
	const char *program_filename;
	const char *parameter_filename;
	const char *firmware_filename;
	struct sigma_firmware_header head;
	struct sigma_action *sa_param;
	struct sigma_action *sa_program;
	size_t param_size;
	size_t program_size;
	uint32_t crc;
	int ret;
	int fd;

	if (argc == 1) {
		parameter_filename = "parameter.bin";
		program_filename = "program.bin";
		firmware_filename = OUTPUT_FILE;
	} else if (argc == 3) {
		parameter_filename = argv[1];
		program_filename = argv[2];
		firmware_filename = OUTPUT_FILE;
	} else if (argc == 4) {
		parameter_filename = argv[1];
		program_filename = argv[2];
		firmware_filename = argv[3];
	} else {
		printf("Usage: %s [<parameter.bin> <program.bin> [<firmware.bin>]]\n", argv[0]);
		exit(1);
	}

	ret = read_file(parameter_filename, sigma_param, &sigma_param_size);
	if (ret)
		return ret;

	ret = read_file(program_filename, sigma_prog, &sigma_program_size);
	if (ret)
		return ret;

	sa_param = sigma_action_alloc(sigma_param_size, sigma_param, &param_size);
	sa_program = sigma_action_alloc(sigma_program_size, sigma_prog, &program_size);

	crc = crc32(sa_program, program_size, 0);
	crc = crc32(sa_param, param_size, crc);


	/* init head */
	memcpy(head.magic, SIGMA_MAGIC, sizeof(SIGMA_MAGIC));
	head.version = 1;
	head.crc = htole32(crc);

	/* write to file */
	fd = open(firmware_filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if(fd < 0) {
		error(0, errno, "Failed to open \"%s\"", firmware_filename);
		return -1;
	}

	write(fd, &head, sizeof(head));
	write(fd, sa_program, program_size);
	write(fd, sa_param, param_size);

	close(fd);

	free(sa_program);
	free(sa_param);

	return 0;
}
