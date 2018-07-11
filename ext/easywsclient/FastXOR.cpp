#include "FastXOR.h"
#include <string.h>

namespace fastxor
{


static inline bool isBigEndian()
{
	int32_t i = 1;
	return *(reinterpret_cast<char*>(&i)) == 0;
}


// XOR this block of memory, in place, by the provided 32 bit key
// Will do it 64 bits at a time up until the end
void fastXOR(void *data, uint32_t dataLen, uint8_t key[4])
{
	if (isBigEndian()) // we don't optimize for big endian processors.
	{
		uint8_t *scan = (uint8_t *)data;
		for (uint8_t i = 0; i < dataLen; i++)
		{
			scan[i] ^= key[i & 3];
		}
	}
	else
	{
		uint8_t *scan = (uint8_t *)data;
		uint32_t blockCount = dataLen / 8;
		// Handle XOR of 8 bytes at a time
		if (blockCount)
		{
			uint64_t mask;
			uint8_t *storeMask = (uint8_t *)&mask;
			// Copy the XOR key into the 64 bit mask value
			memcpy(storeMask, key, 4);
			memcpy(storeMask + 4, key, 4);
			uint64_t *scan64 = (uint64_t *)scan;
			for (uint32_t i = 0; i < blockCount; i++)
			{
				*scan64++ ^= mask;
			}
			scan = (uint8_t *)scan64;
			dataLen -= (blockCount * 8);
		}
		for (uint8_t i = 0; i < dataLen; i++)
		{
			scan[i] ^= key[i & 3];
		}
	}
}

}
