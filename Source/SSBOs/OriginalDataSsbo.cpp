#include "Include/SSBOs/OriginalDataSsbo.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ComputeHeaders/SsboBufferBindings.comp"
#include "Shaders/ComputeHeaders/UniformLocations.comp"

#include "Include/SSBOs/OriginalData.h"

#include <vector>

/*------------------------------------------------------------------------------------------------
Description:
    Initializes base class, then gives derived class members initial values and allocates space 
    for the SSBO.
Parameters: 
    numItems    However many instances of OriginalData the user wants to store.
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
OriginalDataSsbo::OriginalDataSsbo(unsigned int numItems) :
    SsboBase(),  // generate buffers
    _numItems(numItems)
{
    std::vector<OriginalData> v(numItems);

    // now bind this new buffer to the dedicated buffer binding location
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ORIGINAL_DATA_BUFFER_BINDING, _bufferId);

    // this uniform will remain constant after creation (as long as another OriginalDataSsbo 
    // isn't created, which will need a new SSBO and a new shader buffer "header" and new 
    // binding locations)
    glUniform1ui(UNIFORM_LOCATION_ORIGINAL_DATA_BUFFER_SIZE, numItems);

    // and fill it with new data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferId);
    glBufferData(GL_SHADER_STORAGE_BUFFER, v.size() * sizeof(OriginalData), v.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the value that was passed in on creation.
Parameters: None
Returns:    
    See Description.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
unsigned int OriginalDataSsbo::NumItems() const
{
    return _numItems;
}
