#pragma once

// Simple wrapper for socket access
// Hides platform specific details of socket access
#include <stdint.h>

namespace wsocket
{

class Wsocket
{
public:
	// Create's a socket for this hostname and port; returns null if it failed
	static Wsocket *create(const char *hostName,int32_t port);
	// On some platforms the sockets interface has to be manually initialized once on startup and then shutdown
	// These two methods perform that step if needed.
	// If you do not call 'startupSockets' on the windows platform, no socket create call will succeed.
	static void startupSockets(void);
	static void shutdownSockets(void);

	// performs the select operation on this socket
	virtual void select(int32_t timeOut,size_t txBufSize) = 0;

	// Performs a general select on no specific socket 
	virtual void nullSelect(int32_t timeOut) = 0;

	// Receive data from the socket connection.  
	// A return code of -1 means no data received.
	// A return code >0 is number of bytes received.
	virtual int32_t receive(void *dest, uint32_t maxLen) = 0;

	// Send this much data to the socket
	virtual int32_t send(const void *data, uint32_t dataLen) = 0;

	// Close the socket
	virtual void	close(void) = 0;

	// Returns true if the socket send 'would block'
	virtual bool	wouldBlock(void) = 0;

	// Returns true if a socket send is currently 'in progress'
	virtual bool	inProgress(void) = 0;

	// Not sure what this is, but it's in the original code so making it available now.
	virtual void disableNaglesAlgorithm(void) = 0;

	// Close the socket and release this class
	virtual void release(void) = 0;
protected:
	virtual ~Wsocket(void)
	{
	}
};


}; // end of wsocket namespace
