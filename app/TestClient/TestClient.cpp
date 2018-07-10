
#ifdef _MSC_VER
#endif

#include "easywsclient.hpp"
#include "InputLine.h"

int main()
{
	easywsclient::socketStartup();
	easywsclient::WebSocket *ws = easywsclient::WebSocket::create("ws://localhost:3009");
	if (ws)
	{
		char buffer[512];
		uint32_t len = 0;

		InputMode mode = InputMode::NOTHING;
		while (mode != InputMode::ESCAPE)
		{
			mode = getInputLine(buffer, sizeof(buffer), len);
			if (mode == InputMode::ENTER)
			{
				ws->send(std::string(buffer));
				len = 0;
			}
			ws->poll(1); // poll the socket connection

			ws->dispatch([&](const std::string& data)
			{
				printf("Got ASCII message: %s\r\n", data.c_str());
			});


			ws->dispatchBinary([&](const std::vector<uint8_t>& data)
			{
				printf("Got binary data: %d bytes long\r\n", int(data.size()));
			});
		}
		ws->release();
	}
	easywsclient::socketShutdown();


}
