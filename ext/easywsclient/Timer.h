#pragma  once

#include <chrono>

namespace timer 
{

class Timer
{
public:
	Timer()
		: mStartTime(std::chrono::high_resolution_clock::now())
	{ }

	void reset()
	{
		mStartTime = std::chrono::high_resolution_clock::now();
	}

	double getElapsedSeconds()
	{
		auto s = peekElapsedSeconds();
		reset();
		return s;
	}

	double peekElapsedSeconds()
	{
		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = now - mStartTime;
		return diff.count();
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
};

// uses the high resolution timer to approximate a random number.
uint64_t getRandomTime(void)
{
	uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return seed;
}

}


