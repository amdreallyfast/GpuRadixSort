#pragma once

// FreeType requires that its "include" directory be defined at the project level
// Note: FreeType has a single header for everything.  This is evil.
#include <ft2build.h>
#include FT_FREETYPE_H  // also defined relative to "freetype-2.6.1/include/"

#include <string>

/*------------------------------------------------------------------------------------------------
Description:
    Once initialized, it will store everything necessary to render the text of a TrueType font.  
    This should not be accessed directly except for the RenderText(...) function, and even that 
    should be accessed through FreeTypeEncapsulated (may need a better name), which is 
    responsible for storing atlases that are attributed to a specific font size.
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
class FreeTypeAtlas
{
public:
    FreeTypeAtlas(const int uniformTextSamplerLoc, const int uniformTextColorLoc);
    ~FreeTypeAtlas();

    bool Init(const FT_Face face, const int fontPixelHeightSize);

    void RenderText(const std::string &str, const float posScreenCoord[2],
        const float userScale[2], const float color[4]) const;
private:
    // have to reference it on every draw call, so keep it around
    // Note: It is actually a GLuint, which is a typedef of "unsigned int", but I don't want to 
    // include all of the OpenGL declarations in a header file, so just use the original type.
    // It is a tad shady, but keep an eye on build warnings about incompatible types and you
    // should be fine.
    unsigned int _textureId;
    int _textureUnit;

    // the atlas needs access to a vertex buffer
    // Note: Let the atlas make one.  The FreeType encapsulation could do this, but I am hesitant
    // to make a buffer object that would be used by potentially multiple atlases, so let the 
    // atlas make its own.
    unsigned int _vboId;

    // each vertex buffer needs a set of vertex attributes
    unsigned int _vaoId;

    // which sampler to use (0 - GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (??you sure??)
    // Note: Technically it is a GLint, but is simple int here for the same reason 
    // "texture ID" is an unsigned int instead of GLuint.
    // Also Note: The sampler ID is an OpenGL-generated value and is therefore an unsigned int, 
    // but the sampler number must be loaded into the shader as a signed int.  Yay.
    unsigned int _textureSamplerId;
    int _textureSamplerNum;

    // the FreeType glyph set provides the basic printable ASCII character set, and even though 
    // the first 31 are not visible and will therefore not be loaded, the useless bytes are an 
    // acceptable tradeoff for rapid ASCII character lookup
    struct FreeTypeGlyphCharInfo {
        // advance X and Y for screen coordinate calculations
        float ax;
        float ay;

        // bitmap left and top for screen coordinate calculations
        float bl;
        float bt;

        // glyph bitmap width and height for screen coordinate calculations
        float bw;
        float bh;

        // glyph bitmap width and height normalized to [0.0,1.0] for texture coordinate calculations
        // Note: It is better for performance to do the normalization (a division) at startup.
        float nbw;
        float nbh;

        // TODO: change to "texture S" and "texture T" to be clear
        float tx;    // x offset of glyph in texture coordinates (S on range [0.0,1.0])
        float ty;    // y offset of glyph in texture coordinates (T on range [0.0,1.0])
    } _glyphCharInfo[128];

    // the atlas needs to tell the fragment shader which texture sampler and texture color 
    // (FreeType only provides alpha channel) to use, it does that via uniform, and to use 
    // it the atlas should store the uniform's location
    // Note: Actually a GLint.
    // Also Note: The FreeType encapsulation is responsible for the texture atlas' shader 
    // program, and it will provide these values.
    int _uniformTextSamplerLoc;
    int _uniformTextColorLoc;
};

