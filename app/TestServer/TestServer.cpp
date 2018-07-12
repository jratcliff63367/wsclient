
#ifdef _MSC_VER
#endif

#include "easywsclient.h"
#include "wsocket.h"
#include "InputLine.h"

#include <assert.h>
#include <stdio.h>
#include <string>

using easywsclient::WebSocket;


int main()
{
	easywsclient::socketStartup();
	easywsclient::WebSocket *ws = easywsclient::WebSocket::create("ws://localhost:9002");
	assert(ws);
	ws->sendText("goodbye");
	ws->sendText("hello");
	while (ws->getReadyState() != WebSocket::CLOSED) 
	{
		ws->poll(nullptr,1);
	}
	delete ws;

	easywsclient::socketShutdown();

	return 0;
}
