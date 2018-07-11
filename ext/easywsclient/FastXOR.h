#pragma once

#include <stdint.h>

namespace fastxor
{

// XOR this block of memory, in place, by the provided 32 bit key
// Will do it 64 bits at a time up until the end
void fastXOR(void *data,uint32_t dataLen,uint8_t key[4]);

}
