#include "Include/SSBOs/IntermediateDataFirstBuffer.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ComputeHeaders/SsboBufferBindings.comp"
#include "Shaders/ComputeHeaders/UniformLocations.comp"

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
IntermediateDataFirstBuffer::IntermediateDataFirstBuffer(unsigned int numItems) :
    SsboBase(),  // generate buffers
    _numItems(numItems)
{
    std::vector<IntermediateData> v(numItems);

    // now bind this new buffer to the dedicated buffer binding location
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INTERMEDIATE_SORT_FIRST_BUFFER_BINDING, _bufferId);

    // and fill it with new data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferId);
    glBufferData(GL_SHADER_STORAGE_BUFFER, v.size() * sizeof(IntermediateData), v.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/*------------------------------------------------------------------------------------------------
Description:
    Defines the buffer's size uniform in the specified shader.  It uses the #define'd uniform 
    location found in UniformLocations.comp.

    If the shader does not have the uniform or if the shader compiler optimized it out, then 
    OpenGL will complain about not finding it.  Enable debugging in main() in main.cpp for more 
    detail.
Parameters: 
    computeProgramId    Self-explanatory.
Returns:    
    See Description.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
void IntermediateDataFirstBuffer::ConfigureUniforms(unsigned int computeProgramId) const
{
    // the uniform should remain constant after this 
    glUseProgram(computeProgramId);
    glUniform1ui(UNIFORM_LOCATION_INTERMEDIATE_BUFFER_SIZE, _numItems);
    glUseProgram(0);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the value that was passed in on creation.
Parameters: None
Returns:    
    See Description.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
unsigned int IntermediateDataFirstBuffer::NumItems() const
{
    return _numItems;
}

