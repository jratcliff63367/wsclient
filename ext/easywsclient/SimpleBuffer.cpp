#include "SimpleBuffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

namespace simplebuffer
{

	class SimpleBufferImpl : public SimpleBuffer
	{
	public:
		SimpleBufferImpl(uint32_t defaultLen)
		{
			reset(defaultLen);
		}

		virtual ~SimpleBufferImpl(void)
		{
			reset(0);
		}


		// Get the current data buffer.  It can be modified..but...you cannot go beyond
		// the current length
		virtual uint8_t *getData(uint32_t &dataLen) const override final
		{
			dataLen = mLen;
			return mBuffer;
		}

		// Clear the contents of the buffer (simply resets the length back to zero)
		virtual void 		clear(void) override final 	// clear the buffer
		{
			mLen = 0;
		}

		// Add this data to the current buffer.  If 'data' is null, it doesn't copy any data (assuming it was manually written already)
		// but does advance the buffer pointer
		virtual void 		addBuffer(const void *data, uint32_t dataLen) override final
		{
			uint32_t available = mMaxLen - mLen; // how many bytes are currently available...
			// If we are trying to advance past the end of the available buffer space we need to grow the buffer
			if (dataLen > available) // if there isn't enough room, we need to grow the buffer
			{
				// Double the size of buffer
				uint32_t growSize = mMaxLen * 2;
				// If, doubling the size of the buffer still isn't big enough, then grow by at least this data length
				if (growSize < dataLen)
				{
					growSize = dataLen;
				}
				mMaxLen = growSize;		// The new maximum length...
				uint8_t *newBuffer = (uint8_t *)malloc(growSize);
				// If there is any old data to copy over
				if (mLen)
				{
					memcpy(newBuffer, mBuffer, mLen);
				}
				free(mBuffer);
				mBuffer = newBuffer;
				available = mMaxLen - mLen;
			}
			if (dataLen  <= available)
			{
				if (data)
				{
					memcpy(&mBuffer[mLen], data, dataLen);
				}
				mLen += dataLen;
			}
		}

		// Note, the reset command does not retain the previous data buffer!
		virtual void		reset(uint32_t defaultSize) override final
		{
			if (mBuffer)
			{
				free(mBuffer);
				mBuffer = nullptr;
				mLen = 0;
				mMaxLen = 0;
			}
			mMaxLen = defaultSize;
			mLen = 0;
			if (mMaxLen)
			{
				mBuffer = (uint8_t *)malloc(defaultSize);
			}
		}

		// Release the SimpleBuffer instance
		virtual void		release(void) override final
		{
			delete this;
		}

		virtual uint32_t getSize(void) const override final
		{
			return mLen;
		}

		virtual void consume(uint32_t removeLen) override final
		{
			if (removeLen >= mLen)
			{
				mLen = 0;
			}
			else
			{
				uint32_t keepBytes = mLen - removeLen;
				uint32_t startIndex = mLen - keepBytes;
				memcpy(mBuffer, &mBuffer[startIndex], keepBytes);
				mLen = keepBytes;
			}
		}

		// Make sure the buffer is large enough for this capacity; return the *current* read location in the buffer
		virtual	uint8_t	*confirmCapacity(uint32_t capacity) override final
		{
			// Compute how many bytes are available in the current buffer
			uint32_t available = mMaxLen - mLen;
			// If we are asking for more capacity than is available, we need to grow the buffer
			if (capacity > available)
			{
				mMaxLen = mMaxLen * 2; // first try growing by double the current size...
				available = mMaxLen - mLen;
				// If, even after doubling the current size, there still isn't enough room for this capacity
				// We need to adjust the maximum length to support that capacity request
				if (available < capacity)
				{
					mMaxLen += capacity;
				}
				// Allocate a new buffer for the resized data
				uint8_t *newBuffer = (uint8_t *)malloc(mMaxLen);
				// If there was any previous data in the old buffer, we copy it
				if (mLen)
				{
					memcpy(newBuffer, mBuffer, mLen);
				}
				free(mBuffer);	// Free the old buffer
				mBuffer = newBuffer;	// Use the new buffer
			}
			return &mBuffer[mLen];
		}

	private:
		uint8_t		*mBuffer{ nullptr };
		uint32_t	mMaxLen{ 0 };
		uint32_t	mLen{ 0 };

	};

SimpleBuffer *SimpleBuffer::create(uint32_t defaultSize)
{
	auto ret = new SimpleBufferImpl(defaultSize);
	return static_cast<SimpleBuffer *>(ret);
}


}


