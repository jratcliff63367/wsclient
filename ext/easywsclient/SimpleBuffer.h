#pragma once

#include <stdint.h>

namespace simplebuffer
{

class SimpleBuffer
{
public:

	static SimpleBuffer *create(uint32_t defaultSize);

	// Conume this many bytes of the current buffer; retaining whatever is left
	virtual void consume(uint32_t removeLen) = 0;

	virtual uint32_t getSize(void) const = 0;

	// Get the current data buffer.  It can be modified..but...you cannot go beyond
	// the current length
	virtual uint8_t *getData(uint32_t &dataLen) const = 0;

	// Clear the contents of the buffer (simply resets the length back to zero)
	virtual void 		clear(void) = 0; 	// clear the buffer

	// Add this data to the current buffer.  If 'data' is null, it doesn't copy any data
	virtual void 		addBuffer(const void *data,uint32_t dataLen) = 0;

	// Make sure the buffer is large enough for this capacity; return the *current* read location in the buffer
	virtual	uint8_t	*confirmCapacity(uint32_t capacity) = 0;

	// Note, the reset command does not retain the previous data buffer!
	virtual void		reset(uint32_t defaultSize) = 0;

	// Release the SimpleBuffer instance
	virtual void		release(void) = 0;

protected:
	virtual ~SimpleBuffer(void)
	{
	}

};

}
