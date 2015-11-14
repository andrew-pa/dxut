//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

// Helper class for animation and simulation timing.
class StepTimer
{
public:
	StepTimer() :
		elapsedTicks(0),
		totalTicks(0),
		leftOverTicks(0),
		frameCount(0),
		framesPerSecond(0),
		framesThisSecond(0),
		qpcSecondCounter(0),
		isFixedTimeStep(false),
		targetElapsedTicks(TicksPerSecond / 60)
	{
		QueryPerformanceFrequency(&qpcFrequency);
		QueryPerformanceCounter(&qpcLastTime);

		// Initialize max delta to 1/10 of a second.
		qpcMaxDelta = qpcFrequency.QuadPart / 10;
	}

	// Get elapsed time since the previous Update call.
	UINT64 GetElapsedTicks() const						{ return elapsedTicks; }
	double GetElapsedSeconds() const					{ return TicksToSeconds(elapsedTicks); }

	// Get total time since the start of the program.
	UINT64 GetTotalTicks() const						{ return totalTicks; }
	double GetTotalSeconds() const						{ return TicksToSeconds(totalTicks); }

	// Get total number of updates since start of the program.
	UINT32 GetFrameCount() const						{ return frameCount; }

	// Get the current framerate.
	UINT32 GetFramesPerSecond() const					{ return framesPerSecond; }

	// Set whether to use fixed or variable timestep mode.
	void SetFixedTimeStep(bool isFixedTimestep)			{ isFixedTimeStep = isFixedTimestep; }

	// Set how often to call Update when in fixed timestep mode.
	void SetTargetElapsedTicks(UINT64 targetElapsed)	{ targetElapsedTicks = targetElapsed; }
	void SetTargetElapsedSeconds(double targetElapsed)	{ targetElapsedTicks = SecondsToTicks(targetElapsed); }

	// Integer format represents time using 10,000,000 ticks per second.
	static const UINT64 TicksPerSecond = 10000000;

	static double TicksToSeconds(UINT64 ticks)			{ return static_cast<double>(ticks) / TicksPerSecond; }
	static UINT64 SecondsToTicks(double seconds)		{ return static_cast<UINT64>(seconds * TicksPerSecond); }

	// After an intentional timing discontinuity (for instance a blocking IO operation)
	// call this to avoid having the fixed timestep logic attempt a set of catch-up 
	// Update calls.

	void ResetElapsedTime()
	{
		QueryPerformanceCounter(&qpcLastTime);

		leftOverTicks = 0;
		framesPerSecond = 0;
		framesThisSecond = 0;
		qpcSecondCounter = 0;
	}

	typedef void(*LPUPDATEFUNC) (void);

	// Update timer state, calling the specified Update function the appropriate number of times.
	void Tick(LPUPDATEFUNC update)
	{
		// Query the current time.
		LARGE_INTEGER currentTime;

		QueryPerformanceCounter(&currentTime);

		UINT64 timeDelta = currentTime.QuadPart - qpcLastTime.QuadPart;

		qpcLastTime = currentTime;
		qpcSecondCounter += timeDelta;

		// Clamp excessively large time deltas (e.g. after paused in the debugger).
		if (timeDelta > qpcMaxDelta)
		{
			timeDelta = qpcMaxDelta;
		}

		// Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
		timeDelta *= TicksPerSecond;
		timeDelta /= qpcFrequency.QuadPart;

		UINT32 lastFrameCount = frameCount;

		if (isFixedTimeStep)
		{
			// Fixed timestep update logic

			// If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
			// the clock to exactly match the target value. This prevents tiny and irrelevant errors
			// from accumulating over time. Without this clamping, a game that requested a 60 fps
			// fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
			// accumulate enough tiny errors that it would drop a frame. It is better to just round 
			// small deviations down to zero to leave things running smoothly.

			if (abs(static_cast<int>(timeDelta - targetElapsedTicks)) < TicksPerSecond / 4000)
			{
				timeDelta = targetElapsedTicks;
			}

			leftOverTicks += timeDelta;

			while (leftOverTicks >= targetElapsedTicks)
			{
				elapsedTicks = targetElapsedTicks;
				totalTicks += targetElapsedTicks;
				leftOverTicks -= targetElapsedTicks;
				frameCount++;

				if (update)
				{
					update();
				}
			}
		}
		else
		{
			// Variable timestep update logic.
			elapsedTicks = timeDelta;
			totalTicks += timeDelta;
			leftOverTicks = 0;
			frameCount++;

			if (update)
			{
				update();
			}
		}

		// Track the current framerate.
		if (frameCount != lastFrameCount)
		{
			framesThisSecond++;
		}

		if (qpcSecondCounter >= static_cast<UINT64>(qpcFrequency.QuadPart))
		{
			framesPerSecond = framesThisSecond;
			framesThisSecond = 0;
			qpcSecondCounter %= qpcFrequency.QuadPart;
		}
	}

private:
	// Source timing data uses QPC units.
	LARGE_INTEGER qpcFrequency;
	LARGE_INTEGER qpcLastTime;
	UINT64 qpcMaxDelta;

	// Derived timing data uses a canonical tick format.
	UINT64 elapsedTicks;
	UINT64 totalTicks;
	UINT64 leftOverTicks;

	// Members for tracking the framerate.
	UINT32 frameCount;
	UINT32 framesPerSecond;
	UINT32 framesThisSecond;
	UINT64 qpcSecondCounter;

	// Members for configuring fixed timestep mode.
	bool isFixedTimeStep;
	UINT64 targetElapsedTicks;
};
