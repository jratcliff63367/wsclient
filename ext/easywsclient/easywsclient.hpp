#ifndef EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD
#define EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD

// This code comes from:
// https://github.com/dhbaird/easywsclient
//
// To get the latest version:
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.hpp
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.cpp

#include <string>
#include <vector>

namespace easywsclient 
{

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

    // Factories:
    static WebSocket *create(const char *url, const char *origin="",bool useMask=true);

    // Interfaces:
    virtual void poll(WebSocketCallback *callback,int32_t timeout = 0) = 0; // timeout in milliseconds

    virtual void send(const std::string& message) = 0;
    virtual void sendBinary(const std::string& message) = 0;
    virtual void sendBinary(const std::vector<uint8_t>& message) = 0;
    virtual void sendPing() = 0;
    virtual void close() = 0;

    virtual ReadyStateValues getReadyState() const = 0;

	virtual void release(void) = 0;

  protected:
	  virtual ~WebSocket(void)
	  {

	  }
};


void socketStartup(void);
void socketShutdown(void);

} // namespace easywsclient

#endif /* EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD */
