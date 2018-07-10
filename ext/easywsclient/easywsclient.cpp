#ifdef _MSC_VER
#pragma warning(disable:4267 4100 4456 4244)
#endif

#include <vector>
#include <string>

#include "easywsclient.hpp"
#include "wplatform.h"
#include "wsocket.h"

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
			uint32_t	header_size;
			bool		fin;
			bool		mask;
			enum opcode_type
			{
				CONTINUATION = 0x0,
				TEXT_FRAME = 0x1,
				BINARY_FRAME = 0x2,
				CLOSE = 8,
				PING = 9,
				PONG = 0xa,
			} opcode;
			int			N0;
			uint64_t	N;
			uint8_t		masking_key[4];
		};

		WebSocketImpl(wsocket::Wsocket *_sockfd, bool useMask) : mSocket(_sockfd), mReadyState(OPEN), mUseMake(useMask)
		{
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
				mSocket->select(timeout, mTransmitBuffer.size());
			}
			while (true)
			{
				// FD_ISSET(0, &rfds) will be true
				int N = mReceiveBuffer.size();
				mReceiveBuffer.resize(N + 1500);
				int32_t ret = mSocket->receive((char *)&mReceiveBuffer[0] + N, 1500);
				if (false)
				{
				}
				else if (ret < 0 && (mSocket->wouldBlock() || mSocket->inProgress()))
				{
					mReceiveBuffer.resize(N);
					break;
				}
				else if (ret <= 0)
				{
					mReceiveBuffer.resize(N);
					mSocket->close();
					mReadyState = CLOSED;
					fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
					break;
				}
				else
				{
					mReceiveBuffer.resize(N + ret);
				}
			}

			while (mTransmitBuffer.size())
			{
				int ret = mSocket->send(&mTransmitBuffer[0], mTransmitBuffer.size());
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
					mTransmitBuffer.erase(mTransmitBuffer.begin(), mTransmitBuffer.begin() + ret);
				}
			}
			if (!mTransmitBuffer.size() && mReadyState == CLOSING)
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
			// TODO: consider acquiring a lock on mReceiveBuffer...
			while (true)
			{
				wsheader_type ws;
				if (mReceiveBuffer.size() < 2) { return; /* Need at least 2 */ }
				const uint8_t * data = (uint8_t *)&mReceiveBuffer[0]; // peek, but don't consume
				ws.fin = (data[0] & 0x80) == 0x80;
				ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
				ws.mask = (data[1] & 0x80) == 0x80;
				ws.N0 = (data[1] & 0x7f);
				ws.header_size = 2 + (ws.N0 == 126 ? 2 : 0) + (ws.N0 == 127 ? 8 : 0) + (ws.mask ? 4 : 0);
				if (mReceiveBuffer.size() < ws.header_size) { return; /* Need: ws.header_size - mReceiveBuffer.size() */ }
				int i = 0;
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

				if (mReceiveBuffer.size() < ws.header_size + ws.N) { return; /* Need: ws.header_size+ws.N - mReceiveBuffer.size() */ }

				// We got a whole message, now do something with it:
				if (false)
				{
				}
				else if (ws.opcode == wsheader_type::TEXT_FRAME
					|| ws.opcode == wsheader_type::BINARY_FRAME
					|| ws.opcode == wsheader_type::CONTINUATION)
				{
					if (ws.mask)
					{
						for (size_t i = 0; i != ws.N; ++i)
						{
							mReceiveBuffer[i + ws.header_size] ^= ws.masking_key[i & 0x3];
						}
					}
					mReceivedData.insert(mReceivedData.end(), mReceiveBuffer.begin() + ws.header_size, mReceiveBuffer.begin() + ws.header_size + (size_t)ws.N);// just feed
					if (ws.fin)
					{
						if (callback && !mReceivedData.empty())
						{
							callback->receiveMessage(&mReceivedData[0], uint32_t(mReceivedData.size()), ws.opcode == wsheader_type::TEXT_FRAME);
						}
						mReceivedData.erase(mReceivedData.begin(), mReceivedData.end());
						std::vector<uint8_t>().swap(mReceivedData);// free memory
					}
				}
				else if (ws.opcode == wsheader_type::PING)
				{
					if (ws.mask)
					{
						for (size_t i = 0; i != ws.N; ++i)
						{
							mReceiveBuffer[i + ws.header_size] ^= ws.masking_key[i & 0x3];
						}
					}
					std::string data(mReceiveBuffer.begin() + ws.header_size, mReceiveBuffer.begin() + ws.header_size + (size_t)ws.N);
					sendData(wsheader_type::PONG, data.size(), data.begin(), data.end());
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

				mReceiveBuffer.erase(mReceiveBuffer.begin(), mReceiveBuffer.begin() + ws.header_size + (size_t)ws.N);
			}
		}

		void sendPing()
		{
			std::string empty;
			sendData(wsheader_type::PING, empty.size(), empty.begin(), empty.end());
		}

		void send(const std::string& message)
		{
			sendData(wsheader_type::TEXT_FRAME, message.size(), message.begin(), message.end());
		}

		void sendBinary(const std::string& message)
		{
			sendData(wsheader_type::BINARY_FRAME, message.size(), message.begin(), message.end());
		}

		void sendBinary(const std::vector<uint8_t>& message)
		{
			sendData(wsheader_type::BINARY_FRAME, message.size(), message.begin(), message.end());
		}

		template<class Iterator>
		void sendData(wsheader_type::opcode_type type, uint64_t message_size, Iterator message_begin, Iterator message_end)
		{
			// TODO:
			// Masking key should (must) be derived from a high quality random
			// number generator, to mitigate attacks on non-WebSocket friendly
			// middleware:
			const uint8_t masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };
			// TODO: consider acquiring a lock on mTransmitBuffer...
			if (mReadyState == CLOSING || mReadyState == CLOSED)
			{
				return;
			}
			std::vector<uint8_t> header;
			header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (mUseMake ? 4 : 0), 0);
			header[0] = 0x80 | type;
			if (false)
			{
			}
			else if (message_size < 126)
			{
				header[1] = (message_size & 0xff) | (mUseMake ? 0x80 : 0);
				if (mUseMake)
				{
					header[2] = masking_key[0];
					header[3] = masking_key[1];
					header[4] = masking_key[2];
					header[5] = masking_key[3];
				}
			}
			else if (message_size < 65536)
			{
				header[1] = 126 | (mUseMake ? 0x80 : 0);
				header[2] = (message_size >> 8) & 0xff;
				header[3] = (message_size >> 0) & 0xff;
				if (mUseMake)
				{
					header[4] = masking_key[0];
					header[5] = masking_key[1];
					header[6] = masking_key[2];
					header[7] = masking_key[3];
				}
			}
			else
			{ // TODO: run coverage testing here
				header[1] = 127 | (mUseMake ? 0x80 : 0);
				header[2] = (message_size >> 56) & 0xff;
				header[3] = (message_size >> 48) & 0xff;
				header[4] = (message_size >> 40) & 0xff;
				header[5] = (message_size >> 32) & 0xff;
				header[6] = (message_size >> 24) & 0xff;
				header[7] = (message_size >> 16) & 0xff;
				header[8] = (message_size >> 8) & 0xff;
				header[9] = (message_size >> 0) & 0xff;
				if (mUseMake)
				{
					header[10] = masking_key[0];
					header[11] = masking_key[1];
					header[12] = masking_key[2];
					header[13] = masking_key[3];
				}
			}
			// N.B. - mTransmitBuffer will keep growing until it can be transmitted over the socket:
			mTransmitBuffer.insert(mTransmitBuffer.end(), header.begin(), header.end());
			mTransmitBuffer.insert(mTransmitBuffer.end(), message_begin, message_end);
			if (mUseMake)
			{
				size_t message_offset = mTransmitBuffer.size() - message_size;
				for (size_t i = 0; i != message_size; ++i)
				{
					mTransmitBuffer[message_offset + i] ^= masking_key[i & 0x3];
				}
			}
		}

		void close()
		{
			if (mReadyState == CLOSING || mReadyState == CLOSED)
			{
				return;
			}
			mReadyState = CLOSING;
			uint8_t closeFrame[6] = { 0x88, 0x80, 0x00, 0x00, 0x00, 0x00 }; // last 4 bytes are a masking key
			std::vector<uint8_t> header(closeFrame, closeFrame + 6);
			mTransmitBuffer.insert(mTransmitBuffer.end(), header.begin(), header.end());
		}
	private:
		std::vector<uint8_t> mReceiveBuffer;			// receive buffer
		std::vector<uint8_t> mTransmitBuffer;			// transmit buffer
		std::vector<uint8_t> mReceivedData;	// received data

		wsocket::Wsocket	*mSocket{ nullptr };
		ReadyStateValues	mReadyState{ CLOSED };
		bool				mUseMake{ true };
};

WebSocket *WebSocket::create(const char *_url, const char *_origin,bool useMask)
{
	std::string url(_url);
	std::string origin(_origin);

	char host[128];
	int port;
	char path[128];
	if (url.size() >= 128)
	{
		fprintf(stderr, "ERROR: url size limit exceeded: %s\n", url.c_str());
		return NULL;
	}
	if (origin.size() >= 200)
	{
		fprintf(stderr, "ERROR: origin size limit exceeded: %s\n", origin.c_str());
		return NULL;
	}
	if (false)
	{
	}
	else if (sscanf(url.c_str(), "ws://%[^:/]:%d/%s", host, &port, path) == 3)
	{
	}
	else if (sscanf(url.c_str(), "ws://%[^:/]/%s", host, path) == 2)
	{
		port = 80;
	}
	else if (sscanf(url.c_str(), "ws://%[^:/]:%d", host, &port) == 2)
	{
		path[0] = '\0';
	}
	else if (sscanf(url.c_str(), "ws://%[^:/]", host) == 1)
	{
		port = 80;
		path[0] = '\0';
	}
	else
	{
		fprintf(stderr, "ERROR: Could not parse WebSocket url: %s\n", url.c_str());
		return NULL;
	}
	//fprintf(stderr, "easywsclient: connecting: host=%s port=%d path=/%s\n", host, port, path);
	wsocket::Wsocket *sockfd = wsocket::Wsocket::create(host, port);
	if (sockfd == nullptr)
	{
		fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
		return NULL;
	}
	{
		// XXX: this should be done non-blocking,
		char line[256];
		int status;
		int i;
		wplatform::stringFormat(line, 256, "GET /%s HTTP/1.1\r\n", path);
		sockfd->send(line, strlen(line));
		if (port == 80)
		{
			wplatform::stringFormat(line, 256, "Host: %s\r\n", host);
			sockfd->send(line, strlen(line));
		}
		else
		{
			wplatform::stringFormat(line, 256, "Host: %s:%d\r\n", host, port);
			sockfd->send(line, strlen(line));
		}
		wplatform::stringFormat(line, 256, "Upgrade: websocket\r\n");
		sockfd->send(line, strlen(line));
		wplatform::stringFormat(line, 256, "Connection: Upgrade\r\n");
		sockfd->send(line, strlen(line));
		if (!origin.empty())
		{
			wplatform::stringFormat(line, 256, "Origin: %s\r\n", origin.c_str());
			sockfd->send(line, strlen(line));
		}
		wplatform::stringFormat(line, 256, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n");
		sockfd->send(line, strlen(line));
		wplatform::stringFormat(line, 256, "Sec-WebSocket-Version: 13\r\n");
		sockfd->send(line, strlen(line));
		wplatform::stringFormat(line, 256, "\r\n");
		sockfd->send(line, strlen(line));
		for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i)
		{
			if (sockfd->receive(line + i, 1) == 0)
			{
				return NULL;
			}
		}
		line[i] = 0;
		if (i == 255)
		{
			fprintf(stderr, "ERROR: Got invalid status line connecting to: %s\n", url.c_str());
			return NULL;
		}
		if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101)
		{
			fprintf(stderr, "ERROR: Got bad status connecting to %s: %s", url.c_str(), line);
			return NULL;
		}
		// TODO: verify response headers,
		while (true)
		{
			for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i)
			{
				if (sockfd->receive(line + i, 1) == 0)
				{
					return NULL;
				}
			}
			if (line[0] == '\r' && line[1] == '\n')
			{
				break;
			}
		}
	}
	sockfd->disableNaglesAlgorithm();

	auto ret = new WebSocketImpl(sockfd, useMask);
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
