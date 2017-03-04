#include "Include/RenderFrameRate/FreeTypeEncapsulated.h"

// the OpenGL version include also includes all previous versions
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
#include "ThirdParty/glload/include/glload/gl_4_4.h"

// for make the shaders
#include "Shaders/ShaderStorage.h"


/*------------------------------------------------------------------------------------------------
Description:
    Gives member values default values.
Parameters: None
Returns:    None
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
FreeTypeEncapsulated::FreeTypeEncapsulated()
    :
    _haveInitialized(0),
    _programId(0),
    _uniformTextSamplerLoc(0),
    _uniformTextColorLoc(0)
{
}

/*------------------------------------------------------------------------------------------------
Description:
    ??do something? it shouldn't delete the program ID because that is the the shader storage's 
    job??
Parameters: None
Returns:    None
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
FreeTypeEncapsulated::~FreeTypeEncapsulated()
{
}

/*------------------------------------------------------------------------------------------------
Description:
    Finds the needed uniforms in the FreeType shader program and initializes the FreeType 
    library itself, but does not create any atlases.
Parameters: None
Returns:    
    True if all went well, false if there were problems.  Writes error messages to stderr.
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
bool FreeTypeEncapsulated::Init(const std::string &trueTypeFontFilePath, const unsigned int programId)
{
    if (programId == 0)
    {
        fprintf(stderr, "FreeType was given a shader program ID of 0.  This is bad.\n");
        return false;
    }
    _programId = programId;

    // pick out the attributes and uniforms used in the FreeType GPU program

    char textTextureName[] = "textureSamplerId";
    _uniformTextSamplerLoc = glGetUniformLocation(_programId, textTextureName);
    if (_uniformTextSamplerLoc == -1)
    {
        fprintf(stderr, "Could not bind uniform '%s'\n", textTextureName);
        return false;
    }

    //char textColorName[] = "color";
    char textColorName[] = "textureColor";
    _uniformTextColorLoc = glGetUniformLocation(_programId, textColorName);
    if (_uniformTextColorLoc == -1)
    {
        fprintf(stderr, "Could not bind uniform '%s'\n", textColorName);
        return false;
    }


    // FreeType needs to load itself into particular variables
    // Note: FT_Init_FreeType(...) returns something called an FT_Error, which VS can't find.
    // Based on the useage, it is assumed that 0 is returned if something went wrong, otherwise
    // non-zero is returned.  That is the only explanation for this kind of condition.
    if (FT_Init_FreeType(&_ftLib))
    {
        fprintf(stderr, "Could not init freetype library\n");
        return false;
    }

    // Note: FT_New_Face(...) also returns an FT_Error.
    if (FT_New_Face(_ftLib, trueTypeFontFilePath.c_str(), 0, &_ftFace))
    {
        fprintf(stderr, "Could not open font '%s'\n", trueTypeFontFilePath.c_str());
        return false;
    }

    _haveInitialized = true;
    return true;
}

/*------------------------------------------------------------------------------------------------
Description:
    Retrieves a FreeTypeAtlas for the given font size.  If there is no atlas available for that 
    font size, it is created.

    Note: Changing the font size will change the size of the bitmaps of each character, so each 
    font size must be associated with its own atlas.  It is possible to scale the texture up and 
    down when drawing, but scaling up too much will making the text pixelated.
Parameters: 
    fontSize    A size in device pixels (device pixels are based on whatever the OS believes to 
                be the current monitor resolution).  Size 48 is pretty good for a simple 
                framerate counter.
Returns:
    A const shared pointer to a FreeTypeAtlas object, or 0 if something went wrong.  It is const 
    so that the user cannot even try to re-initialize it.  Error messages are written to stderr.
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
const std::shared_ptr<FreeTypeAtlas> FreeTypeEncapsulated::GetAtlas(const int fontSize)
{
    if (!_haveInitialized)
    {
        fprintf(stderr, "FreeTypeEncapsulated object has not been initialized with font file and shader file paths.\n");
        return 0;
    }

    _ATLAS_MAP::iterator itr = _atlasMap.find(fontSize);
    if (itr != _atlasMap.end())
    {
        // found it
        return itr->second;
    }
    else
    {
        // make a new one
        std::shared_ptr<FreeTypeAtlas> newAtlasPtr = std::make_shared<FreeTypeAtlas>(
            _uniformTextSamplerLoc, _uniformTextColorLoc);
        if (newAtlasPtr->Init(_ftFace, fontSize))
        {
            _atlasMap[fontSize] = newAtlasPtr;
            return newAtlasPtr;
        }

        // if something went wrong with atlas initialization, so let it die
        fprintf(stderr, "FreeTypeAtlas could not be initialized with font size '%d'\n", fontSize);
        return 0;
    }
}

