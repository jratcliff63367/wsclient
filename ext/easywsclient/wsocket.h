#pragma once

// Simple wrapper for socket access
// Hides platform specific details of socket access
#include <stdint.h>

namespace wsocket
{

class Wsocket
{
public:
	static Wsocket *create(const char *hostName,int32_t port);
	static void startupSockets(void);
	static void shutdownSockets(void);

	virtual void select(uint32_t timeOut,size_t txBufSize) = 0;
	virtual void nullSelect(uint32_t timeOut) = 0;

	virtual int32_t receive(void *dest, uint32_t maxLen) = 0;
	virtual int32_t send(const void *data, uint32_t dataLen) = 0;

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
