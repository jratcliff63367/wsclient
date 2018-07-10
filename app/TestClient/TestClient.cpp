
#ifdef _MSC_VER
#endif

#include "easywsclient.h"
#include "InputLine.h"
#include <stdio.h>

class ReceiveData : public easywsclient::WebSocketCallback
{
public:
	virtual void receiveMessage(const void *data, uint32_t dataLen, bool isAscii) override final
	{
		if (isAscii)
		{
			const char *cdata = (const char *)data;
			printf("Got message:");
			for (uint32_t i = 0; i < dataLen; i++)
			{
				printf("%c", cdata[i]);
			}
			printf("\r\n");
		}
		else
		{
			printf("Got binary data %d bytes long.\r\n", dataLen);
		}
	}
};

int main()
{
	easywsclient::socketStartup();
	easywsclient::WebSocket *ws = easywsclient::WebSocket::create("ws://localhost:3009");
	if (ws)
	{
		char buffer[512];
		uint32_t len = 0;
		ReceiveData rd;
		InputMode mode = InputMode::NOTHING;
		while (mode != InputMode::ESCAPE)
		{
			mode = getInputLine(buffer, sizeof(buffer), len);
			if (mode == InputMode::ENTER)
			{
				ws->sendText(buffer);
				len = 0;
			}
			ws->poll(&rd,1); // poll the socket connection
		}
		ws->release();
	}
	easywsclient::socketShutdown();


}
