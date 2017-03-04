#pragma once

#include <string>

// TODO: header
class ComputeParticleSort
{
public:
    ComputeParticleSort(unsigned int numParticles, const std::string &computeShaderKey);
    ~ComputeParticleSort();

    // TODO: use C++'s std::chrono for microsecond measurement of assigning Morton Codes
    // TODO: repeat for the radix sort
    // TODO: once all Morton Codes are being sorted, go off on a branch and try the approach of "prefix sums", then "scan and sort", then "prefix sums", then "scan and sort", repeat for all 15 bit-pairs (or heck, maybe 10 bit-triplets)
    // TODO: repeat timing for the multi-summon radix sort
    void Sort();

private:

};
