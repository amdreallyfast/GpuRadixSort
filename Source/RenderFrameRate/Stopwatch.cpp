#include "Include/RenderFrameRate/Stopwatch.h"

// this is a big header, but necessary to get access to LARGE_INTEGER
// Note: We can't just include winnt.h, in which LARGE_INTEGER is defined, because there are some macros that this header file needs that are defined further up in the header hierarchy.  So just include Windows.h and be done with it.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h>

// these are declared static here in order to avoid having to include Windows.h in the header
static double gInverseCpuTimerFrequency;
static LARGE_INTEGER gStartCounter;
static LARGE_INTEGER gLastLapCounter;

/*------------------------------------------------------------------------------------------------
Description:
    Converts a CPU cycle counter into fractions of a second.
    counter     A LARGE_INTEGER (Windows-specific union of structs used for large or
    high-precision integer work.
Returns:
    A double indicating the fractions of a second corresponding to the argument counter.  This 
    is not "time elapsed" yet but is rather totally depended upon the "counter" argument.
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
static inline double CounterToSeconds(const LARGE_INTEGER counter)
{
    return ((double)counter.QuadPart * gInverseCpuTimerFrequency);
}

/*------------------------------------------------------------------------------------------------
Description:
    Sets up members to default values.
Parameters: None
Returns:    None
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
Stopwatch::Stopwatch() :
    _haveInitialized(false)
{
    gStartCounter.QuadPart = 0;
    gLastLapCounter.QuadPart = 0;
}

/*------------------------------------------------------------------------------------------------
Description:
    Reads the CPU's frequency.
    Must be called prior to use.  
Parameters: None
Returns:    None
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
void Stopwatch::Init()
{
    // the "performance frequency" only changes on system reset, so it's ok to set it only 
    // during initialization
    // Note: It is technically possible for this query to return 0, but only on old versions of Windows.  According to the documentation, Windows XP or greater will never return 0. 
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms644905(v=vs.85).aspx
    LARGE_INTEGER cpuFreq;
    QueryPerformanceFrequency(&cpuFreq);
    gInverseCpuTimerFrequency = 1.0 / cpuFreq.QuadPart;
    _haveInitialized = true;
}

/*------------------------------------------------------------------------------------------------
Description:
    Reads the CPU's clock counter and assumes it as the starting point for Lap() and TotalTime() 
    method calls.
Parameters: None
Returns:    None
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
void Stopwatch::Start()
{
    if (!_haveInitialized)
    {
        fprintf(stderr, "StopWatch has not been initialized.\n");
    }

    // give the counters their first values
    // Note: "On systems that run Windows XP or later, the function will always succeed and will 
    // thus never return zero."
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx
    QueryPerformanceCounter(&gStartCounter);
    gLastLapCounter.QuadPart = gStartCounter.QuadPart;
}

/*------------------------------------------------------------------------------------------------
Description:
    Reads the CPU's clock counter, compares it to the time set by Start() or the last call to 
    Lap(), and returns the time since that call.
Parameters: None
Returns:    
    Fractions of a seconds since the last call to Start() or Lap().  The fraction can be >1.
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
double Stopwatch::Lap()
{
    if (!_haveInitialized)
    {
        fprintf(stderr, "StopWatch has not been initialized.\n");
    }

    // get the current time
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    // calculate delta time relative to previous frame
    LARGE_INTEGER deltaLargeInt;
    deltaLargeInt.QuadPart = now.QuadPart - gLastLapCounter.QuadPart;
    double deltaTime = CounterToSeconds(deltaLargeInt);

    gLastLapCounter.QuadPart = now.QuadPart;

    return deltaTime;
}

/*------------------------------------------------------------------------------------------------
Description:
    Reads the CPU's clock counter, compares it to the time set by Start() or Reset(), and 
    returns the fractions of a second since then.
Parameters: None
Returns:
    Fractions of a seconds since the last call to Start() or Reset().  The fraction can be >1.
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
double Stopwatch::TotalTime()
{
    if (!_haveInitialized)
    {
        fprintf(stderr, "StopWatch has not been initialized.\n");
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    LARGE_INTEGER deltaLargeInt;
    deltaLargeInt.QuadPart = now.QuadPart - gStartCounter.QuadPart;
    double deltaTime = CounterToSeconds(deltaLargeInt);
    
    return deltaTime;
}

/*------------------------------------------------------------------------------------------------
Description:
    Reads the CPU's clock counter and resets the starting time.
Parameters: None
Returns:    None
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
void Stopwatch::Reset()
{
    if (!_haveInitialized)
    {
        fprintf(stderr, "StopWatch has not been initialized.\n");
    }

    // reset the values by giving them new start values
    Start();
}
