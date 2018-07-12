#include "FastXOR.h"
#include <string.h>
#include <stdio.h>

#define DEBUG_PRINT 0
#define FORCE_ALIGNMENT 1

namespace fastxor
{


static inline bool isBigEndian()
{
	int32_t i = 1;
	return *(reinterpret_cast<char*>(&i)) == 0;
}

static inline uint32_t xorBytes(uint8_t *data, uint32_t dataLen, uint8_t key[4])
{
	uint32_t i;
	for (i = 0; i < dataLen; i++)
	{
		data[i] ^= key[i & 3];
	}
	return i & 3; // returns the key index we ended on
}

void slowXOR(void *data, uint32_t dataLen, uint8_t key[4])
{
	xorBytes((uint8_t*)data, dataLen, key);
}


// XOR this block of memory, in place, by the provided 32 bit key
// Will do it 64 bits at a time up until the end
void fastXOR(void *data, uint32_t dataLen, uint8_t key[4])
{
	if (isBigEndian()) // we don't optimize for big endian processors.
	{
		xorBytes((uint8_t *)data, dataLen, key);
	}
	else
	{
		uint8_t alignKey[4] = { key[0], key[1], key[2], key[3] };
		uint8_t *scan = (uint8_t *)data;
#if FORCE_ALIGNMENT
		uint64_t startAddress = (uint64_t)data;
		uint32_t skipBytes = (uint32_t)(startAddress & 0x7); // how many bytes we have to skip to get to the 
		if (skipBytes)
		{
			skipBytes = 8 - skipBytes;
			if (skipBytes > dataLen)
			{
				skipBytes = dataLen;
			}
#if DEBUG_PRINT
			printf("Skipping the first %d bytes to make sure we are 8 byte aligned.\r\n", skipBytes);
#endif
			uint32_t keyIndex = xorBytes(scan, skipBytes, key);
			alignKey[0] = key[((keyIndex+0) & 3)];
			alignKey[1] = key[((keyIndex + 1) & 3)];
			alignKey[2] = key[((keyIndex + 2) & 3)];
			alignKey[3] = key[((keyIndex + 3) & 3)];
			scan += skipBytes;
			dataLen -= skipBytes;
		}
#endif
		uint32_t blockCount = dataLen / 8;
#if DEBUG_PRINT
		printf("fastXOR: %d bytes blockCount:%d.\r\n", dataLen, blockCount);
#endif
		// Handle XOR of 8 bytes at a time
		if (blockCount)
		{
			uint64_t mask = 0;
			uint8_t *storeMask = (uint8_t *)&mask;
			memcpy(storeMask, alignKey, 4);
			memcpy(storeMask+4,alignKey, 4);
			uint64_t *scan64 = (uint64_t *)scan;
#if DEBUG_PRINT
			printf("Key8 : %02X%02X%02X%02X\r\n", alignKey[3], alignKey[2], alignKey[1], alignKey[0]);
			printf("Key64: %016llX\r\n", (long long unsigned int)mask);
			printf("Scan64: %016llX\r\n", (long long unsigned int)scan64);
#endif
			for (uint32_t i= 0; i < blockCount; i++)
			{
//				printf("XOR block: %d\r\n", i );
				scan64[i] ^= mask;
			}
			scan = (uint8_t *)(scan64+blockCount);
			dataLen -= (blockCount * 8);
#if DEBUG_PRINT
			printf("Finished blocks\r\n");
#endif
		}
		if (dataLen)
		{
#if DEBUG_PRINT
			printf("Doing partial with %d bytes\r\n", dataLen);
#endif
			xorBytes(scan, dataLen, alignKey);
		}
#if DEBUG_PRINT
		printf("Fast XOR Completed.\r\n");
#endif
	}
}

}
