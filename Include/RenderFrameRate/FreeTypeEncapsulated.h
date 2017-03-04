#pragma once

// there doesn't seem to be any way out of including all of FreeType without some shady forward 
// declarations of FreeType internal structures
// Note: FreeType has a single header for everything.  This is evil.
#include <ft2build.h>
#include FT_FREETYPE_H  // also defined relative to "freetype-2.6.1/include/"

#include "Include/RenderFrameRate/FreeTypeAtlas.h"

#include <string>
#include <memory>   // for the shared pointer
#include <map>      // for storing the atlas pointers

/*------------------------------------------------------------------------------------------------
Description:
    Stores TrueType texture atlases (FreeTypeAtlas) that are associated with a particular shader 
    program.  In practice, there will likely be only one shader program for rendering 
    characters, so there will probably only be a single FreeTypeencapsulated object.

    The name may need work.  I didn't know what else to call it because it handles extra-atlas 
    overhead work and I didn't want to call it a "manager", which is equally unhelpful.
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
class FreeTypeEncapsulated
{
public:
    FreeTypeEncapsulated();
    ~FreeTypeEncapsulated();

    bool Init(const std::string &trueTypeFontFilePath, const unsigned int programId);
    const std::shared_ptr<FreeTypeAtlas> GetAtlas(const int fontSize);

private:
    bool _haveInitialized;

    FT_Library _ftLib;  // move to a "FreeTypeContainment" class
    FT_Face _ftFace;    // move to a "FreeTypeContainment" class

    typedef std::map<int, std::shared_ptr<FreeTypeAtlas>> _ATLAS_MAP;
    _ATLAS_MAP _atlasMap;

    // drawing these textures requires their own shader
    // Note: These are actually GLuint and GLint values, but I didn't want to include all of 
    // OpenGL just for the typedefs, which is unfortunately necessary since the only readily 
    // available header files for OpenGL include everything.
    unsigned int _programId;

    // these uniforms are also stored in each created atlas, but they need to be kept around 
    // here because they will need to be passed to each new atlas
    int _uniformTextSamplerLoc;   // uniform location within program
    int _uniformTextColorLoc;     // uniform location within program
};