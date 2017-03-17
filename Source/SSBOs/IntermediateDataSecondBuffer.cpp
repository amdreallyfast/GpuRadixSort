#include "Include/SSBOs/IntermediateDataSecondBuffer.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ComputeHeaders/SsboBufferBindings.comp"

#include "Include/SSBOs/IntermediateData.h"

#include <vector>

/*------------------------------------------------------------------------------------------------
Description:
    Initializes base class, then gives derived class members initial values and allocates space 
    for the SSBO.
Parameters: 
    numItems    MUST be the same size as PrefixScanBuffer::AllPrefixSums.
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
IntermediateDataSecondBuffer::IntermediateDataSecondBuffer(unsigned int numItems) :
    SsboBase()  // generate buffers
{
    std::vector<IntermediateData> v(numItems);

    // now bind this new buffer to the dedicated buffer binding location
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INTERMEDIATE_SORT_SECOND_BUFFER_BINDING, _bufferId);

    // and fill it with new data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferId);
    glBufferData(GL_SHADER_STORAGE_BUFFER, v.size() * sizeof(IntermediateData), v.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
