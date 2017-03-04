#pragma once

/*------------------------------------------------------------------------------------------------
Description:
    Call Init() to have it read the CPU frequency, then Start() to have it register a starting 
    time.  After that, Lap() will get you the number of seconds elapsed since Start() was called.

    This class was ported in from my first game engine that I built while going through some 
    tutorials on the topic.  I have no idea how old it is, but I think that it is from 2014 or 
    2015, which is when I was still learning graphical programming and early stuff on game 
    engines.
Creator:    John Cox (??-2015)
------------------------------------------------------------------------------------------------*/
class Stopwatch
{
public:
    Stopwatch();
    void Init();

    // delta time is in seconds
    void Start();
    double Lap();
    double TotalTime();
    void Reset();
private:
    bool _haveInitialized;
};

