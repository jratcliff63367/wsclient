
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
static WebSocket::pointer ws = NULL;

void handle_message(const std::string & message)
{
	printf(">>> %s\n", message.c_str());
	if (message == "world") { ws->close(); }
}

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

	ws = WebSocket::from_url("ws://localhost:9002");
	assert(ws);
	ws->send("goodbye");
	ws->send("hello");
	while (ws->getReadyState() != WebSocket::CLOSED) 
	{
		ws->poll();
		ws->dispatch(handle_message);
	}
	delete ws;
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
