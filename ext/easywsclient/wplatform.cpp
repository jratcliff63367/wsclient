#include "wplatform.h"
#include <stdio.h>
#include <stdarg.h>

namespace wplatform
{


	int32_t stringFormatV(char* dst, size_t dstSize, const char* src, va_list arg)
	{
		return ::vsnprintf(dst, dstSize, src, arg);
	}

	int32_t  stringFormat(char* dst, size_t dstSize, const char* format, ...)
	{
		va_list arg;
		va_start(arg, format);
		int32_t r = stringFormatV(dst, dstSize, format, arg);
		va_end(arg);
		return r;
	}

}