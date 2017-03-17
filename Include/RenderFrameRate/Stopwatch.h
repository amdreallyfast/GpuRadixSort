#pragma once

#include <chrono>

/*------------------------------------------------------------------------------------------------
Description:
    Uses C++11's std::chrono library to read precise time and provide some convenient 
    time-related services.

    Delta times are in fractions of a second (fraction can be greater than 1).
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class Stopwatch
{
public:
    Stopwatch();

    // also serves as "restart"
    void Start();

    double Lap();
    double TotalTime();
private:
    std::chrono::steady_clock::time_point _startTime;
    std::chrono::steady_clock::time_point _lastLapTime;
};

