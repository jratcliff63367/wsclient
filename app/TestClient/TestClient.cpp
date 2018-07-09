
#ifdef _MSC_VER
#endif

#include "easywsclient.hpp"
#include "InputLine.h"

int main()
{
	easywsclient::socketStartup();
	easywsclient::WebSocket *ws = easywsclient::WebSocket::from_url("ws://localhost:3009");
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
		}
		ws->close();
	}
	easywsclient::socketShutdown();


}
