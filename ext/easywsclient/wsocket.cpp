#include "wsocket.h"
#include "wplatform.h"

#ifdef _WIN32
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS // _CRT_SECURE_NO_WARNINGS for sscanf errors in MSVC2013 Express
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <fcntl.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment( lib, "ws2_32" )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <io.h>
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
#ifndef _SOCKET_T_DEFINED
typedef SOCKET socket_t;
#define _SOCKET_T_DEFINED
#endif
#if _MSC_VER >=1600
// vs2010 or later
#include <stdint.h>
#else
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
#define socketerrno WSAGetLastError()
#define SOCKET_EAGAIN_EINPROGRESS WSAEINPROGRESS
#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#ifndef _SOCKET_T_DEFINED
typedef int socket_t;
#define _SOCKET_T_DEFINED
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif
#define closesocket(s) ::close(s)
#include <errno.h>
#define socketerrno errno
#define SOCKET_EAGAIN_EINPROGRESS EAGAIN
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#endif

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

#define LOG_SENDS 0
#define LOG_RECEIVES 0

namespace wsocket
{

class WsocketImpl : public Wsocket
{
public:
	WsocketImpl(const char *hostName, int32_t port)
	{
		mSocket = hostname_connect(hostName, port);
	}

	virtual ~WsocketImpl(void)
	{
		close();
	}

	virtual void nullSelect(int32_t timeout) override final
	{
		timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
		::select(0, NULL, NULL, NULL, &tv);
	}

	virtual void select(int32_t timeout, size_t txBufSize) override final
	{
		fd_set rfds;
		fd_set wfds;
		timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(mSocket, &rfds);
		if (txBufSize) 
		{ 
			FD_SET(mSocket, &wfds);
		}
		::select(int(mSocket) + 1, &rfds, &wfds, 0, timeout > 0 ? &tv : 0);
	}

	virtual int32_t receive(void *dest, uint32_t maxLen) override final
	{
		int32_t ret = -1;
		if (mSocket)
		{
			ret = ::recv(mSocket, (char *)dest, int(maxLen), 0);
#if LOG_RECEIVES
			if (ret > 0)
			{
				const uint8_t *scan = (const uint8_t *)dest;
				printf("SOCKET_RECEIVE:");
				for (int32_t i = 0; i < ret; i++)
				{
					uint8_t c = scan[i];
					if (c >= 32 && c < 127)
					{
						printf("%c", c);
					}
					else
					{
						uint32_t v = uint32_t(c);
						printf("(%02X)", v);
					}
				}
				printf("\r\n");
			}
#endif
		}
		return ret;
	}

	virtual int32_t send(const void *data, uint32_t dataLen) override final
	{
		int32_t ret = ::send(mSocket, (const char *)data, int(dataLen), 0);
#if LOG_SENDS
		if (ret > 0)
		{
			if (uint32_t(ret) != dataLen)
			{
				printf("*** Partial send: %d bytes requested %d bytes actually send.\r\n", dataLen, ret);
			}
			const uint8_t *scan = (const uint8_t *)data;
			printf("SOCKET_SEND:");
			for (int32_t i = 0; i < ret; i++)
			{
				uint8_t c = scan[i];
				if (c >= 32 && c < 127)
				{
					printf("%c", c);
				}
				else
				{
					uint32_t v = uint32_t(c);
					printf("(%02X)", v);
				}
			}
			printf("\r\n");
		}
#endif
		return ret;
	}

	// Not sure what this is, but it's in the original code so making it available now.
	virtual void disableNaglesAlgorithm(void) override final
	{
		int flag = 1;
		setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)); // Disable Nagle's algorithm
#ifdef _WIN32
		u_long on = 1;
		ioctlsocket(mSocket, FIONBIO, &on);
#else
		fcntl(mSocket, F_SETFL, O_NONBLOCK);
#endif
	}

	virtual void release(void) override final
	{
		delete this;
	}

	bool isValid(void) const
	{
		return mSocket != INVALID_SOCKET;
	}

	virtual void close(void) override final
	{
		if (mSocket)
		{
			closesocket(mSocket);
		}
		mSocket = 0;
	}

	virtual bool	wouldBlock(void) override final
	{
		return socketerrno == SOCKET_EWOULDBLOCK;
	}

	virtual bool	inProgress(void) override final
	{
		return socketerrno == SOCKET_EAGAIN_EINPROGRESS;
	}


	socket_t hostname_connect(const char *hostname, int port)
	{
		addrinfo hints;
		addrinfo *result;
		addrinfo *p;
		int ret;
		socket_t sockfd = INVALID_SOCKET;
		char sport[16];
		memset(&hints, 0, sizeof(hints));

		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		wplatform::stringFormat(sport, 16, "%d", port);

		if ((ret = getaddrinfo(hostname, sport, &hints, &result)) != 0)
		{
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
			return 1;
		}
		for (p = result; p != NULL; p = p->ai_next)
		{
			sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (sockfd == INVALID_SOCKET)
			{
				continue;
			}
			if (connect(sockfd, p->ai_addr, int(p->ai_addrlen)) != SOCKET_ERROR)
			{
				break;
			}
			closesocket(sockfd);
			sockfd = INVALID_SOCKET;
		}
		freeaddrinfo(result);
		return sockfd;
	}

	socket_t	mSocket{ INVALID_SOCKET };
};

Wsocket *Wsocket::create(const char *hostName, int32_t port)
{
	auto ret = new WsocketImpl(hostName, port);
	if (!ret->isValid())
	{
		delete ret;
		ret = nullptr;
	}
	return static_cast<Wsocket *>(ret);
}

void Wsocket::startupSockets(void)
{
#ifdef _WIN32
	INT rc;
	WSADATA wsaData;

	rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

}

void Wsocket::shutdownSockets(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}

}