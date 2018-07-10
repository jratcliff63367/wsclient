#pragma  once

// This code comes from:
// https://github.com/jratcliff63367/wsclient
// It *originally* came from easywsclient but has been modified
// https://github.com/dhbaird/easywsclient
//
#include <stdint.h>

namespace easywsclient 
{

// Pure virtual callback interface to receive messages from the server.
class WebSocketCallback
{
public:
	virtual void receiveMessage(const void *data, uint32_t dataLen, bool isAscii) = 0;
};

class WebSocket 
{
public:
	enum ReadyStateValues 
	{ 
		CLOSING, 
		CLOSED, 
		CONNECTING, 
		OPEN 
	};

	// Factor method to create an instance of the websockets client
	// 'url' is the URL we are connecting to.
	// 'origin' is the optional origin
	// useMask should be true, it mildly XOR encrypts all messages
	static WebSocket *create(const char *url, const char *origin="",bool useMask=true);

	// This client is polled from a single thread.  These methods are *not* thread safe.
	// If you call them from different threads, you will need to create your own mutex.
	// While this 'poll' call is non-blocking, you can still run the whole socket connection in it's own
	// thread.  This is recommended for maximum performance
	// Calling the 'poll' routine will process all sends and receives
	// If any new messages have been received from the sever and you have provided a valid 'callback' pointer, then
	// it will send incoming messages back through that interface
	virtual void poll(WebSocketCallback *callback,int32_t timeout = 0) = 0; // timeout in milliseconds

	// Send a text message to the server.  Assumed zero byte terminated ASCIIZ string
	virtual void sendText(const char *str) = 0;

	// Send a binary message with explicit length provided.
	virtual void sendBinary(const void *data,uint32_t dataLen) = 0;

	// Ping the server
	virtual void sendPing() = 0;

	// Close the connection
	virtual void close() = 0;

	// Retreive the current state of the connection
	virtual ReadyStateValues getReadyState() const = 0;

	// Release the websockets client interface class
	virtual void release(void) = 0;

  protected:
	// Dummy protected destructor. Do not try to 'delete' this class, instead call 'release' to dispose of it
	virtual ~WebSocket(void)
	{
	}
};

// Initialize sockets one time for your app.
void socketStartup(void);

// Shutdown sockets on exit from your app
void socketShutdown(void);

} // namespace easywsclient

