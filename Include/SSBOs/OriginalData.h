#pragma once

/*------------------------------------------------------------------------------------------------
Description:
    For this demo, the "original data" structure is simply an integer.  But the framework exists 
    for sorting whatever.

    There is only a single integer here, so there should be no implicit padding.  Padding only 
    happens with a value that does end on a 4-word alignment is succeeded by a value that will 
    put it over the 4-word alignment.  A single integer structure should have no trouble being 
    put back to back with other such structures.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
struct OriginalData
{
    /*--------------------------------------------------------------------------------------------
    Description:
        Initializes members to 0.
    Parameters: None
    Returns:    None
    Creator:    John Cox, 3/2017
    --------------------------------------------------------------------------------------------*/
    OriginalData::OriginalData() :
        _value(0)
    {
    }

    // must be unsigned because Radix Sorting fails when it runs across a bit that does not 
    // contribute to the value's quantity
    unsigned int _value;
};