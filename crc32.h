#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdlib.h>
#include <stdint.h>

uint32_t crc32(const void *buf, size_t size, uint32_t crc);

#endif
