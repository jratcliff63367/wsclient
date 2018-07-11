#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "easywsclient.h"
#include "wplatform.h"
#include "wsocket.h"
#include "SimpleBuffer.h"

#define DEFAULT_TRANSMIT_BUFFER_SIZE (1024*16)	// Default transmit buffer size is 16k
#define DEFAULT_RECEIVE_BUFFER_SIZE (1024*16)	// Default transmit buffer size is 16k
#define DEFAULT_MAX_READ_SIZE (1024*16)			// Maximum size of a single read operation

namespace easywsclient
{ // private module-only namespace

	class WebSocketImpl : public easywsclient::WebSocket
	{
	public:
		// http://tools.ietf.org/html/rfc6455#section-5.2  Base Framing Protocol
		//
		//  0                   1                   2                   3
		//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		// +-+-+-+-+-------+-+-------------+-------------------------------+
		// |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
		// |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
		// |N|V|V|V|       |S|             |   (if payload len==126/127)   |
		// | |1|2|3|       |K|             |                               |
		// +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
		// |     Extended payload length continued, if payload len == 127  |
		// + - - - - - - - - - - - - - - - +-------------------------------+
		// |                               |Masking-key, if MASK set to 1  |
		// +-------------------------------+-------------------------------+
		// | Masking-key (continued)       |          Payload Data         |
		// +-------------------------------- - - - - - - - - - - - - - - - +
		// :                     Payload Data continued ...                :
		// + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
		// |                     Payload Data continued ...                |
		// +---------------------------------------------------------------+
		struct wsheader_type
		{
			uint32_t	header_size;		// Size of the header
			bool		fin;				// Whether or not this is the final block
			bool		mask;				// whether or not this frame uses masking
			enum opcode_type
			{
				CONTINUATION	= 0,
				TEXT_FRAME		= 1,
				BINARY_FRAME	= 2,
				CLOSE			= 8,
				PING			= 9,
				PONG			= 10
			} opcode;
			int32_t		N0;
			uint64_t	N;
			uint8_t		masking_key[4];		// Masking key used for this frame
		};

		WebSocketImpl(const char *url,const char *origin, bool useMask) : mReadyState(OPEN), mUseMask(useMask)
		{
			mTransmitBuffer = simplebuffer::SimpleBuffer::create(DEFAULT_TRANSMIT_BUFFER_SIZE);
			mReceivedData	= simplebuffer::SimpleBuffer::create(DEFAULT_RECEIVE_BUFFER_SIZE);
			mReceiveBuffer	= simplebuffer::SimpleBuffer::create(DEFAULT_RECEIVE_BUFFER_SIZE);

			size_t urlSize = strlen(url);
			size_t originSize = strlen(origin);
			char host[128];
			int port=0;
			char path[128];
			if (urlSize >= 128)
			{
				fprintf(stderr, "ERROR: url size limit exceeded: %s\n", url);
			}
			else if (originSize >= 200)
			{
				fprintf(stderr, "ERROR: origin size limit exceeded: %s\n", origin);
			}
			else
			{
				if (sscanf(url, "ws://%[^:/]:%d/%s", host, &port, path) == 3)
				{
				}
				else if (sscanf(url, "ws://%[^:/]/%s", host, path) == 2)
				{
					port = 80;
				}
				else if (sscanf(url, "ws://%[^:/]:%d", host, &port) == 2)
				{
					path[0] = '\0';
				}
				else if (sscanf(url, "ws://%[^:/]", host) == 1)
				{
					port = 80;
					path[0] = '\0';
				}
				else
				{
					fprintf(stderr, "ERROR: Could not parse WebSocket url: %s\n", url);
				}
				if (port)
				{
					fprintf(stderr, "easywsclient: connecting: host=%s port=%d path=/%s\n", host, port, path);
					mSocket = wsocket::Wsocket::create(host, port);
					if (mSocket == nullptr)
					{
						fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
					}
					else
					{
						// XXX: this should be done non-blocking,
						char line[256];
						int status;
						int i;
						wplatform::stringFormat(line, 256, "GET /%s HTTP/1.1\r\n", path);
						mSocket->send(line, uint32_t(strlen(line)));
						if (port == 80)
						{
							wplatform::stringFormat(line, 256, "Host: %s\r\n", host);
							mSocket->send(line, uint32_t(strlen(line)));
						}
						else
						{
							wplatform::stringFormat(line, 256, "Host: %s:%d\r\n", host, port);
							mSocket->send(line, uint32_t(strlen(line)));
						}
						wplatform::stringFormat(line, 256, "Upgrade: websocket\r\n");
						mSocket->send(line, uint32_t(strlen(line)));
						wplatform::stringFormat(line, 256, "Connection: Upgrade\r\n");
						mSocket->send(line, uint32_t(strlen(line)));
						if (originSize)
						{
							wplatform::stringFormat(line, 256, "Origin: %s\r\n", origin);
							mSocket->send(line, uint32_t(strlen(line)));
						}
						wplatform::stringFormat(line, 256, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n");
						mSocket->send(line, uint32_t(strlen(line)));
						wplatform::stringFormat(line, 256, "Sec-WebSocket-Version: 13\r\n");
						mSocket->send(line, uint32_t(strlen(line)));
						wplatform::stringFormat(line, 256, "\r\n");
						mSocket->send(line, uint32_t(strlen(line)));
						for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i)
						{
							if (mSocket->receive(line + i, 1) == 0)
							{
								mSocket->release();
								mSocket = nullptr;
								break;
							}
						}
						line[i] = 0;
						if (i == 255)
						{
							fprintf(stderr, "ERROR: Got invalid status line connecting to: %s\n", url);
							mSocket->release();
							mSocket = nullptr;
						}
						if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101)
						{
							fprintf(stderr, "ERROR: Got bad status connecting to %s: %s", url, line);
							mSocket->release();
							mSocket = nullptr;
						}
						if (mSocket)
						{
							// TODO: verify response headers,
							while (true)
							{
								for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i)
								{
									if (mSocket->receive(line + i, 1) == 0)
									{
										mSocket->release();
										mSocket = nullptr;
										break;
									}
								}
								if (line[0] == '\r' && line[1] == '\n')
								{
									break;
								}
							}
						}
					}
					if (mSocket)
					{
						mSocket->disableNaglesAlgorithm();
					}
				}
			}
		}

		virtual ~WebSocketImpl(void)
		{
			close();
			while (mReadyState != CLOSED)
			{
				poll(nullptr, 1);
			}
			if (mSocket)
			{
				mSocket->release();
			}
			if (mReceivedData)
			{
				mReceivedData->release();
			}
			if (mReceiveBuffer)
			{
				mReceiveBuffer->release();
			}
			if (mTransmitBuffer)
			{
				mTransmitBuffer->release();
			}
		}

		virtual void release(void) override final
		{
			delete this;
		}

		ReadyStateValues getReadyState() const
		{
			return mReadyState;
		}

		void poll(WebSocketCallback *callback, int timeout)
		{ // timeout in milliseconds
			if (mReadyState == CLOSED)
			{
				if (timeout > 0)
				{
					mSocket->nullSelect(timeout);
				}
				return;
			}
			if (timeout != 0)
			{
				mSocket->select(timeout, mTransmitBuffer->getSize());
			}
			while (true)
			{
				uint8_t *rbuffer = mReceiveBuffer->confirmCapacity(DEFAULT_MAX_READ_SIZE);
				int32_t ret = mSocket->receive(rbuffer, DEFAULT_MAX_READ_SIZE);
				if (ret < 0 && (mSocket->wouldBlock() || mSocket->inProgress()))
				{
					break;
				}
				else if (ret <= 0)
				{
					mSocket->close();
					mReadyState = CLOSED;
					fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
					break;
				}
				else
				{
					// Advance the buffer pointer by the number of bytes read
					mReceiveBuffer->addBuffer(nullptr, ret);
				}
			}

			while (mTransmitBuffer->getSize())
			{
				uint32_t dataLen;
				const uint8_t *buffer = mTransmitBuffer->getData(dataLen);
				int ret = mSocket->send(buffer, dataLen);
				if (false)
				{
				} // ??
				else if (ret < 0 && (mSocket->wouldBlock() || mSocket->inProgress()))
				{
					break;
				}
				else if (ret <= 0)
				{
					mSocket->close();
					mReadyState = CLOSED;
					fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
					break;
				}
				else
				{
					mTransmitBuffer->consume(ret); // shrink the transmit buffer by the number of bytes we managed to send..
				}
			}
			if (!mTransmitBuffer->getSize() && mReadyState == CLOSING)
			{
				mSocket->close();
				mReadyState = CLOSED;
			}
			if (callback)
			{
				_dispatchBinary(callback);
			}
		}

		virtual void _dispatchBinary(WebSocketCallback *callback)
		{
			while (true)
			{
				wsheader_type ws;
				uint32_t dataLen;
				uint8_t *data = mReceiveBuffer->getData(dataLen);

				if (dataLen < 2) 
				{ 
					break;
				}
				ws.fin		= (data[0] & 0x80) == 0x80;
				ws.opcode	= (wsheader_type::opcode_type) (data[0] & 0x0f);
				ws.mask		= (data[1] & 0x80) == 0x80;
				ws.N0		= (data[1] & 0x7f);
				ws.header_size = 2 + (ws.N0 == 126 ? 2 : 0) + (ws.N0 == 127 ? 8 : 0) + (ws.mask ? 4 : 0);

				if (dataLen < ws.header_size) 
				{ 
					break;
				}
				int32_t i = 0;
				if (ws.N0 < 126)
				{
					ws.N = ws.N0;
					i = 2;
				}
				else if (ws.N0 == 126)
				{
					ws.N = 0;
					ws.N |= ((uint64_t)data[2]) << 8;
					ws.N |= ((uint64_t)data[3]) << 0;
					i = 4;
				}
				else if (ws.N0 == 127)
				{
					ws.N = 0;
					ws.N |= ((uint64_t)data[2]) << 56;
					ws.N |= ((uint64_t)data[3]) << 48;
					ws.N |= ((uint64_t)data[4]) << 40;
					ws.N |= ((uint64_t)data[5]) << 32;
					ws.N |= ((uint64_t)data[6]) << 24;
					ws.N |= ((uint64_t)data[7]) << 16;
					ws.N |= ((uint64_t)data[8]) << 8;
					ws.N |= ((uint64_t)data[9]) << 0;
					i = 10;
				}
				if (ws.mask)
				{
					ws.masking_key[0] = ((uint8_t)data[i + 0]) << 0;
					ws.masking_key[1] = ((uint8_t)data[i + 1]) << 0;
					ws.masking_key[2] = ((uint8_t)data[i + 2]) << 0;
					ws.masking_key[3] = ((uint8_t)data[i + 3]) << 0;
				}
				else
				{
					ws.masking_key[0] = 0;
					ws.masking_key[1] = 0;
					ws.masking_key[2] = 0;
					ws.masking_key[3] = 0;
				}

				if (dataLen < ws.header_size + ws.N) 
				{ 
					break;
				}

				// We got a whole message, now do something with it:
				if (ws.opcode == wsheader_type::TEXT_FRAME
					|| ws.opcode == wsheader_type::BINARY_FRAME
					|| ws.opcode == wsheader_type::CONTINUATION)
				{
					// TODO: Optimize this!!
					if (ws.mask)
					{
						for (size_t j = 0; j != ws.N; ++j)
						{
							data[j + ws.header_size] ^= ws.masking_key[j & 0x3];
						}
					}
					// If we are finished and there is no previous received data we can avoid a memory
					// copy by just calling back directly with this receive buffer
					if (ws.fin && mReceivedData->getSize() == 0)
					{
						if (callback )
						{
							callback->receiveMessage(data+ws.header_size, uint32_t(ws.N), ws.opcode == wsheader_type::TEXT_FRAME);
						}
					}
					else
					{
						// Add the input data to the receive data frame we are accumulating
						mReceivedData->addBuffer(data + ws.header_size, uint32_t(ws.N));
						if (ws.fin)
						{
							if (callback && mReceivedData->getSize())
							{
								uint32_t dlen;
								const void *rdata = mReceivedData->getData(dlen);
								callback->receiveMessage(rdata, dlen, ws.opcode == wsheader_type::TEXT_FRAME);
							}
							mReceivedData->clear();
						}
					}
				}
				else if (ws.opcode == wsheader_type::PING)
				{
					if (ws.mask)
					{
						for (size_t j = 0; j != ws.N; ++j)
						{
							data[j + ws.header_size] ^= ws.masking_key[j & 0x3];
						}
					}
					const uint8_t *pingData = nullptr;
					if (ws.N)
					{
						pingData = data + ws.header_size;
					}
					sendData(wsheader_type::PONG, pingData, ws.N);
				}
				else if (ws.opcode == wsheader_type::PONG)
				{
				}
				else if (ws.opcode == wsheader_type::CLOSE)
				{
					close();
				}
				else
				{
					fprintf(stderr, "ERROR: Got unexpected WebSocket message.\n");
					close();
				}
				mReceiveBuffer->clear();
			}
		}

		void sendPing()
		{
			sendData(wsheader_type::PING, nullptr, 0);
		}

		virtual void sendText(const char *str) override final
		{
			size_t len = str ? strlen(str) : 0;
			sendData(wsheader_type::TEXT_FRAME, str, len);
		}

		virtual void sendBinary(const void *data, uint32_t dataLen) override final
		{
			sendData(wsheader_type::BINARY_FRAME, data, dataLen);
		}

		// Just get the high resolution timer as the current masking key
		// we just take the bottom 32 bits of the current high resolution time
		// It doesn't have the most entropy in the world, but it's good enough for
		// this use case and is reasonably fast and portable.
		inline void getMaskingKey(uint8_t maskingKey[4])
		{
			uint64_t seed = wplatform::getRandomTime();
			memcpy(maskingKey, &seed, 4);
		}

		void sendData(wsheader_type::opcode_type type,	// Type of data we are sending
					  const void *messageData,			// The optional message data (this can be null)
					  uint64_t message_size)			// The size of the message data
		{
			uint8_t masking_key[4];
			getMaskingKey(masking_key);

			// TODO: consider acquiring a lock on mTransmitBuffer...
			if (mReadyState == CLOSING || mReadyState == CLOSED)
			{
				return;
			}
			uint8_t header[14];

			uint32_t expectedHeaderLen = 2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (mUseMask ? 4 : 0);
			uint32_t headerLen = expectedHeaderLen;

			header[0] = uint8_t(0x80 | type);
			if (message_size < 126)
			{
				header[1] = (message_size & 0xff) | (mUseMask ? 0x80 : 0);
				if (mUseMask)
				{
					header[2] = masking_key[0];
					header[3] = masking_key[1];
					header[4] = masking_key[2];
					header[5] = masking_key[3];
					headerLen = 6;
				}
				else
				{
					headerLen = 2;
				}
			}
			else if (message_size < 65536)
			{
				header[1] = 126 | (mUseMask ? 0x80 : 0);
				header[2] = (message_size >> 8) & 0xff;
				header[3] = (message_size >> 0) & 0xff;
				if (mUseMask)
				{
					header[4] = masking_key[0];
					header[5] = masking_key[1];
					header[6] = masking_key[2];
					header[7] = masking_key[3];
					headerLen = 8;
				}
				else
				{
					headerLen = 4;
				}
			}
			else
			{ // TODO: run coverage testing here
				header[1] = 127 | (mUseMask ? 0x80 : 0);
				header[2] = (message_size >> 56) & 0xff;
				header[3] = (message_size >> 48) & 0xff;
				header[4] = (message_size >> 40) & 0xff;
				header[5] = (message_size >> 32) & 0xff;
				header[6] = (message_size >> 24) & 0xff;
				header[7] = (message_size >> 16) & 0xff;
				header[8] = (message_size >> 8) & 0xff;
				header[9] = (message_size >> 0) & 0xff;
				if (mUseMask)
				{
					header[10] = masking_key[0];
					header[11] = masking_key[1];
					header[12] = masking_key[2];
					header[13] = masking_key[3];
					headerLen = 14;
				}
				else
				{
					headerLen = 10;
				}
			}
			assert(headerLen == expectedHeaderLen);
			// N.B. - mTransmitBuffer will keep growing until it can be transmitted over the socket:
			mTransmitBuffer->addBuffer(header, headerLen);
			if (messageData)
			{
				mTransmitBuffer->addBuffer(messageData, uint32_t(message_size));
			}
			// If we are using masking then we need to XOR the message by the mask
			if (mUseMask)
			{
				// TODO: This needs to be optimized
				uint32_t message_offset = mTransmitBuffer->getSize() - uint32_t(message_size);
				uint32_t dataLen;
				uint8_t *data = mTransmitBuffer->getData(dataLen);
				uint8_t *maskData = &data[message_offset];
				for (uint32_t i = 0; i != message_size; ++i)
				{
					maskData[i] ^= masking_key[i & 0x3];
				}
			}
		}

		void close()
		{
			if (mReadyState == CLOSING || mReadyState == CLOSED)
			{
				return;
			}
			// add the 'close frame' command to the transmit buffer and set the closing state on
			mReadyState = CLOSING;
			uint8_t closeFrame[6] = { 0x88, 0x80, 0x00, 0x00, 0x00, 0x00 }; // last 4 bytes are a masking key
			mTransmitBuffer->addBuffer(closeFrame, sizeof(closeFrame));
		}

		bool isValid(void) const
		{
			return mSocket ? true : false;
		}

	private:
		simplebuffer::SimpleBuffer	*mReceiveBuffer{ nullptr };		// receive buffer
		simplebuffer::SimpleBuffer	*mTransmitBuffer{ nullptr };	// transmit buffer
		simplebuffer::SimpleBuffer	*mReceivedData{ nullptr };		// received data
		wsocket::Wsocket			*mSocket{ nullptr };
		ReadyStateValues			mReadyState{ CLOSED };
		bool						mUseMask{ true };
};

WebSocket *WebSocket::create(const char *url, const char *origin,bool useMask)
{
	auto ret = new WebSocketImpl(url, origin, useMask);
	if (!ret->isValid())
	{
		ret->release();
		ret = nullptr;
	}
	return static_cast<WebSocket *>(ret);
}

void socketStartup(void)
{
	wsocket::Wsocket::startupSockets();
}

void socketShutdown(void)
{
	wsocket::Wsocket::shutdownSockets();
}

} // namespace easywsclient
