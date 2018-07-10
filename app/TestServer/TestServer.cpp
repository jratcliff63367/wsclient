
#ifdef _MSC_VER
#endif

#include "easywsclient/easywsclient.hpp"
#include "InputLine.h"

#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string>

using easywsclient::WebSocket;


int main()
{
#ifdef _WIN32
	INT rc;
	WSADATA wsaData;

	rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rc) 
	{
		printf("WSAStartup Failed.\n");
		return 1;
	}
#endif

	easywsclient::WebSocket *ws = easywsclient::WebSocket::create("ws://localhost:9002");
	assert(ws);
	ws->sendText("goodbye");
	ws->sendText("hello");
	while (ws->getReadyState() != WebSocket::CLOSED) 
	{
		ws->poll(nullptr,1);
	}
	ws->release();
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
