#include "SimpleBuffer.h"
#include <stdlib.h>
#include <string.h>


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

		// Add this data to the current buffer.  If 'data' is null, it doesn't copy any data
		virtual void 		addBuffer(const void *data, uint32_t dataLen) override final
		{
			uint32_t available = mMaxLen - mLen; // how many bytes are currently available...
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

		virtual void shrink(uint32_t removeLen) override final
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


