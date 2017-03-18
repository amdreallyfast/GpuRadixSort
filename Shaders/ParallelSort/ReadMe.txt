The intermediate sort buffers and PrefixScanBuffer::AllPrefixSums buffer are the same size.


DataToIntermediateDataForSorting.comp 
- launched with 1 thread for each item in PrefixScanBuffer::AllPrefixSums
- excess threads create IntermediateData structures with value of maximum uint so that these entries will stay at the back after sorting sorted to the back.
- reading from OriginalDataBuffer.comp
- fill out the first of buffers in IntermediateSortBuffers.comp

Set IntermediateSortBuffers.comp's uReadFromFirstBuffer to 1.  

radix sort loop through 32 bits
{
    GetNextBitForPrefixSums.comp
    - launched with 1 thread for each item in the PrefixScanBuffer (or size of IntermediateSortBuffers; both are same size)
    - reads from the "read" buffer in IntermediateSortBuffers.comp
    - "write" index is the current thread's global ID
    - uses uBitNumber and the value of whatever IntermediateData structure the current thread is reading to pluck out a 0 or 1
    - puts the 0 or 1 in the PrefixScanBuffer::AllPrefixSums 
    
    ParallelPrefixScan.comp
    - set uCalculateAll to 1
    - launch with half the number of threads as the size of the PrefixScanBuffer::AllPrefixSums (the algorithm requires that each thread handle 2 items)
    - set uCalculateAll to 0
    - launch again with 1 work group worth of threads
    
    SortIntermediateDataUsingPrefixSums.comp
    - launch with 1 thread for each item in the PrefixScanBuffer
    - reads from the "read" buffer in IntermediateSortBuffers.comp
    - uses prefix sum of the thread's work group (PrefixScanBuffer::PerGroupSums) + the prefix sum within the thread's work group (PrefixScanBuffer::AllPrefixSums) to calculate the destination index
    - copy the thread's IntermediateData structure from the IntermediateSortBuffers' "read" buffer to the "write" buffer
    
    Switch Set IntermediateSortBuffers.comp's uReadFromFirstBuffer (if 1 set to 0; if 0, set to 1)
}

glCopyBufferSubData(...) from OriginalDataBuffer to OriginalDataBufferCopyForSorting

SortDataWithSortedIntermediateData.comp
- launched with 1 thread for each item in OriginalDataBuffer (NOT 1 for each item in PrefixScanBuffer::AllPrefixSums like DataToIntermediateDataForSorting.comp did)
- copy the original data structure from OriginalDataBufferCopyForSorting (index from IntermediateData structure) to OriginalDataBuffer (index is current thread's global ID)

