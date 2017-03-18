#pragma once

#include <memory>
#include <string>

#include "Include/SSBOs/SsboBase.h"
#include "Include/SSBOs/PrefixSumSsbo.h"
#include "Include/SSBOs/IntermediateDataFirstBuffer.h"
#include "Include/SSBOs/IntermediateDataSecondBuffer.h"
#include "Include/SSBOs/OriginalDataCopySsbo.h"


// TODO: header
// The sorting process involves multiple compute shaders in a particular order.  This class handles them.
class ParallelSort
{
public:
    ParallelSort(unsigned int originalDataSize);

    // TODO: use C++'s std::chrono for microsecond measurement of assigning Morton Codes
    // TODO: repeat for the radix sort
    // TODO: once all Morton Codes are being sorted, go off on a branch and try the approach of "prefix sums", then "scan and sort", then "prefix sums", then "scan and sort", repeat for all 15 bit-pairs (or heck, maybe 10 bit-triplets)
    // TODO: repeat timing for the multi-summon radix sort
    void Sort();
    //void Sort(const SsboBase *originalDataBuffer);

private:
    unsigned int _originalDataToIntermediateDataProgramId;
    unsigned int _getBitForPrefixSumsProgramId;
    unsigned int _parallelPrefixScanProgramId;

    std::unique_ptr<OriginalDataCopySsbo> _originalDataCopySsbo;
    std::unique_ptr<IntermediateDataFirstBuffer> _intermediateDataFirstBuffer;
    std::unique_ptr<IntermediateDataSecondBuffer> _intermediateDataSecondBuffer;
    std::unique_ptr<PrefixSumSsbo> _prefixSumSsbo;
};
