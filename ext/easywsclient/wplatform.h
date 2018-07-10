#pragma once

#include <stdint.h>

// Platform specific support functions.
// Currently just a string formatting function

namespace wplatform
{

	int32_t  stringFormat(char* dst, size_t dstSize, const char* format, ...);

	// uses the high resolution timer to approximate a random number.
	uint64_t getRandomTime(void);

}
