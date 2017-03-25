#include "Include/RenderFrameRate/Stopwatch.h"


/*------------------------------------------------------------------------------------------------
Description:
    ??anything??
Parameters: None
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
Stopwatch::Stopwatch()
{
    // std::chrono items have their own initializers to 0 (I think)
}

/*------------------------------------------------------------------------------------------------
Description:
    Records a starting point for both Lap() and TotalTime().
Parameters: None
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
void Stopwatch::Start()
{
    _startTime = std::chrono::high_resolution_clock::now();
    _lastLapTime = _startTime;
}

/*------------------------------------------------------------------------------------------------
Description:
    Gives the delta time in fractions of a second since the last call to Start() (if this is the 
    first time that Lap() is called since then) or since the last call to Lap() (if this is not 
    the first time that Lap() has been called since Start()).
Parameters: None
Returns:    
    See Description.  Value is a double to handle very small values.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
double Stopwatch::Lap()
{
    using namespace std::chrono;

    // there is no performance penalty to using high_resolution_clock over something else (like 
    // steady_clock) it always reads the CPU's clock counter and then converts that into 
    // whatever time span the user wants 
    steady_clock::time_point currentTime = high_resolution_clock::now();
    unsigned int deltaTime = duration_cast<microseconds>(currentTime - _lastLapTime).count();
    _lastLapTime = currentTime;

    // time is in microseconds, so divide by 1 million to get fraction of a second
    double secondFraction = (double)deltaTime / 1000000.0;
    return secondFraction;
}

/*------------------------------------------------------------------------------------------------
Description:
    Gives the delta time in fractions of a second since the last call to Start().

    Same procedure as Lap(), but compared with _startTime instead of _lastLapTime.
Parameters: None
Returns:
    See Description.  Value is a double to handle very small values.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
double Stopwatch::TotalTime()
{
    using namespace std::chrono;
    steady_clock::time_point currentTime = high_resolution_clock::now();
    unsigned int microsecondsPassed =
        duration_cast<microseconds>(currentTime - _startTime).count();

    double secondFraction = (double)microsecondsPassed / 1000000.0;
    return secondFraction;
}

