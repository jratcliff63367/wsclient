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

namespace easywsclient {

struct Callback_Imp { virtual void operator()(const std::string& message) = 0; };
struct BytesCallback_Imp { virtual void operator()(const std::vector<uint8_t>& message) = 0; };

class WebSocket 
{
  public:
    enum readyStateValues 
	{ 
		CLOSING, 
		CLOSED, 
		CONNECTING, 
		OPEN 
	};

    // Factories:
    static WebSocket *create(const char *url, const char *origin="",bool useMask=true);

    // Interfaces:
    virtual void poll(int timeout = 0) = 0; // timeout in milliseconds
    virtual void send(const std::string& message) = 0;
    virtual void sendBinary(const std::string& message) = 0;
    virtual void sendBinary(const std::vector<uint8_t>& message) = 0;
    virtual void sendPing() = 0;
    virtual void close() = 0;
    virtual readyStateValues getReadyState() const = 0;

    template<class Callable>
    void dispatch(Callable callable)
        // For callbacks that accept a string argument.
    { // N.B. this is compatible with both C++11 lambdas, functors and C function pointers
        struct _Callback : public Callback_Imp 
		{
            Callable& callable;
            _Callback(Callable& callable) : callable(callable) 
			{ 
			}
            void operator()(const std::string& message) 
			{ 
				callable(message); 
			}
        };
        _Callback callback(callable);
        _dispatch(callback);
    }

    template<class Callable>
    void dispatchBinary(Callable callable)
        // For callbacks that accept a std::vector<uint8_t> argument.
    { // N.B. this is compatible with both C++11 lambdas, functors and C function pointers
        struct _Callback : public BytesCallback_Imp {
            Callable& callable;
            _Callback(Callable& callable) : callable(callable) { }
            void operator()(const std::vector<uint8_t>& message) { callable(message); }
        };
        _Callback callback(callable);
        _dispatchBinary(callback);
    }

	virtual void release(void) = 0;

  protected:
	  virtual ~WebSocket(void)
	  {

	  }
    virtual void _dispatch(Callback_Imp& callable) = 0;
    virtual void _dispatchBinary(BytesCallback_Imp& callable) = 0;
};


void socketStartup(void);
void socketShutdown(void);

} // namespace easywsclient

#endif /* EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD */
