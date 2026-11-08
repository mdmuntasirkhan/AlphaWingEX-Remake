#ifndef TIMER_H
#define TIMER_H
#include <SDL.h>


class Timer {
private:
	Uint64 prevTicks;
	Uint64 currentTicks;

	Timer(const Timer&)				= delete;
	Timer(Timer&&)					= delete;
	Timer& operator=(const Timer&)  = delete;
	Timer& operator=(Timer&&)		= delete;

public:
	Timer();
	~Timer();

	void Start();
	void UpdateFrameTicks();

	float GetDeltaTime() const;
	unsigned int GetSleepTime(const unsigned int fps_) const;
	float GetCurrentTicks() const;
};

#endif // TIMER_H
