/**
 * Copyright (C) 2009-2012 Analog Devices, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *
 **/
#ifndef __GEN_FIRMWARE_H__
#define __GEN_FIRMWARE_H__

#include <stdint.h>

#define SIGMA_MAGIC "ADISIGM"
/* 8192 may not be enough in the future */
#define MAX_LEN (8192)
#define OUTPUT_FILE "SigmaDSP_fw.bin"

struct sigma_firmware_header {
	uint8_t magic[7];
	uint8_t version;
	uint32_t crc;
};

enum {
	SIGMA_ACTION_WRITEXBYTES = 0,
	SIGMA_ACTION_WRITESINGLE,
	SIGMA_ACTION_WRITESAFELOAD,
	SIGMA_ACTION_DELAY,
	SIGMA_ACTION_PLLWAIT,
	SIGMA_ACTION_NOOP,
	SIGMA_ACTION_END,
};

struct sigma_action {
	uint8_t instr;
	uint8_t len_hi;
	uint16_t len;
	uint16_t addr;
	uint8_t payload[];
};

#endif
