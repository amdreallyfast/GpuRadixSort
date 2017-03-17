#pragma once

/*------------------------------------------------------------------------------------------------
Description:
    Make sure that it matches the structure of the one with the same name in 
    IntermediateSortBuffers.comp.

    Note: Not padding is necessary because two of these structures back to back are 4-word 
    aligned, so GLSL shouldn't mess us up.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
struct IntermediateData
{
    /*--------------------------------------------------------------------------------------------
    Description:
        Initializes members to 0.
    Parameters: None
    Returns:    None
    Creator:    John Cox, 3/2017
    --------------------------------------------------------------------------------------------*/
    IntermediateData::IntermediateData() :
        _data(0),
        _globalIndexOfOriginalData(0)
    {
    }

    unsigned int _data;
    unsigned int _globalIndexOfOriginalData;

};