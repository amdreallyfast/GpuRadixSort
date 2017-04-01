#pragma once

#include <memory>
#include <string>

#include "Include/SSBOs/SsboBase.h"
#include "Include/SSBOs/PrefixSumSsbo.h"
#include "Include/SSBOs/IntermediateDataSsbo.h"
#include "Include/SSBOs/OriginalDataSsbo.h"
#include "Include/SSBOs/OriginalDataCopySsbo.h"


// TODO: header
// The sorting process involves multiple compute shaders in a particular order.  This class handles them.
class ParallelSort
{
public:
    ParallelSort(const OriginalDataSsbo::SHARED_PTR &dataToSort);

    void Sort();

private:
    unsigned int _originalDataToIntermediateDataProgramId;
    unsigned int _getBitForPrefixScansProgramId;
    unsigned int _parallelPrefixScanProgramId;
    unsigned int _sortIntermediateDataProgramId;
    unsigned int _sortOriginalDataProgramId;

    // these are unique to this class and are needed for sorting
    OriginalDataCopySsbo::SHARED_PTR _originalDataCopySsbo;
    IntermediateDataSsbo::SHARED_PTR _intermediateDataSsbo;
    PrefixSumSsbo::SHARED_PTR _prefixSumSsbo;

    // need to keep this around until the end of Sort() in order to copy the sorted data back to the original buffer
    OriginalDataSsbo::SHARED_PTR _originalDataSsbo;

};
