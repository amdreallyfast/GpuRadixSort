#pragma once

#include <string>

#include "Include/SSBOs/PrefixSumSsbo.h"


// TODO: header
// The sorting process involves multiple compute shaders in a particular order.  This class handles them.
class ParticleSort
{
public:
    ParticleSort(unsigned int numParticles);
    ~ParticleSort();

    // TODO: use C++'s std::chrono for microsecond measurement of assigning Morton Codes
    // TODO: repeat for the radix sort
    // TODO: once all Morton Codes are being sorted, go off on a branch and try the approach of "prefix sums", then "scan and sort", then "prefix sums", then "scan and sort", repeat for all 15 bit-pairs (or heck, maybe 10 bit-triplets)
    // TODO: repeat timing for the multi-summon radix sort
    void Sort(int numParticles);

private:
    // TODO: make this a shared pointer
    PrefixSumSsbo *_prefixSumSsbo;
    unsigned int _parallelPrefixScanComputeProgramId;
    int _unifLocMaxThreadCount;
    int _unifLocCalculateAll;

};
