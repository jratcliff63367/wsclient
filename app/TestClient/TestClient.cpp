
#ifdef _MSC_VER
#endif

#include "easywsclient.h"
#include "InputLine.h"
#include "wplatform.h"
#include "TestSharedMemory.h"

#include <stdio.h>
#include <string.h>

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

int main(int argc,const char **argv)
{
//	testSharedMemory();

	if (argc != 2)
	{
		printf("Usage: TestClient <hostName>\r\n");
		printf("\r\n");
		printf("Use 'localhost' for current machine\r\n");
		printf("Type: 'bye' or 'quit' or 'exit' to close the client out.\r\n");
	}
	else
	{
		easywsclient::socketStartup();
		char connectString[512];
		wplatform::stringFormat(connectString, sizeof(connectString), "ws://%s:3009", argv[1]);
		easywsclient::WebSocket *ws = easywsclient::WebSocket::create(connectString);
		if (ws)
		{
			printf("Type: 'bye' or 'quit' or 'exit' to close the client out.\r\n");
			inputline::InputLine *inputLine = inputline::InputLine::create();
			ReceiveData rd;
			bool keepRunning = true;
			while (keepRunning)
			{
				const char *data = inputLine->getInputLine();
				if (data)
				{
					if (strcmp(data, "bye") == 0 || strcmp(data, "exit") == 0 || strcmp(data, "quit") == 0)
					{
						keepRunning = false;
					}
					ws->sendText(data);
				}
				ws->poll(&rd, 1); // poll the socket connection
			}
			delete ws;
		}
		easywsclient::socketShutdown();
	}
}
