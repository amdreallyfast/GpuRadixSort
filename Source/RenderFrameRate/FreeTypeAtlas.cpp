#include "Include/RenderFrameRate/FreeTypeAtlas.h"

// the OpenGL version include also includes all previous versions
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
#include "ThirdParty/glload/include/glload/gl_4_4.h"

// needed to get the width and height of the rendering window for calculations of screen 
// coordinate/pixel count fractions
// Build note: Must be included after OpenGL code (in this case, glload).
// Build note: Also need to link freeglut/lib/freeglutD.lib.  However, the linker will try to 
// find "freeglut.lib" (note the lack of "D") instead unless the following preprocessor 
// directives are set either here or in the source-building command line (VS has a
// "Preprocessor" section under "C/C++" for preprocessor definitions).  This is true for every
// source file that wants to use freeglut, so the source-building command line is a useful tool.
// However, since this is bare bones and attempts to avoid any VS-specific project stuff or
// solution stuff, I am taking the verbose path.
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "ThirdParty/freeglut/include/GL/freeglut.h"

#include <algorithm>    // for std::max
#include <vector>       // for memory safe allocation of coordinate info for each glyph

struct point {
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
};

/*------------------------------------------------------------------------------------------------
Description:
    Initializes members to default values.  In this demo program, these are handled by a 
    FreeTypeEncapsulate object.
Parameters:
    uniformTextSamplerLoc   The location of the texture sampler variable in the FreeType shader 
                            program.
    uniformTextColorLoc     The location of the color sampler variable in the FreeType shader 
                            program.
Returns:    None
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
FreeTypeAtlas::FreeTypeAtlas(const int uniformTextSamplerLoc, const int uniformTextColorLoc) :
    _textureId(0),
    _textureUnit(0),
    _vboId(0),
    _vaoId(0),
    _textureSamplerId(0),
    _textureSamplerNum(0),
    _uniformTextSamplerLoc(uniformTextSamplerLoc),
    _uniformTextColorLoc(uniformTextColorLoc)
{
    // clear out the character memory to all 0s (standard practice for arrays)
    memset(_glyphCharInfo, 0, sizeof(_glyphCharInfo));
}

/*------------------------------------------------------------------------------------------------
Description:
    This class generates and stores its own texture and vertex buffer, so it is responsible for 
    cleaning them up when it is done.
Parameters: None
Returns:    None
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
FreeTypeAtlas::~FreeTypeAtlas()
{
    glDeleteTextures(1, &_textureId);
    glDeleteBuffers(1, &_vboId);
}

/*------------------------------------------------------------------------------------------------
Description:
    Creates the texture (the "atlas") for the font's typeface, fills it in, then creates the 
    vertex buffer storage and its associated vertex array object for the quads that will be the 
    surface on which glyph textures will draw.
Parameters:
    FT_Face                 A typedef for a pointer (gah).  From the declaration for it: "A 
                            handle to a given typographic face object.  A face object models a 
                            given typeface, in a given style."  Provided by FreetypeEncapsulate.
    fontPixelHeightSize     Determines the size of the glyph bitmaps.  Necessary to allocate the 
                            proper number of bytes on the GPU.
Returns:
    True if all went well, otherwise false.  At this time (8-20-2016), there is no way for it 
    return false.
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
bool FreeTypeAtlas::Init(const FT_Face face, const int fontPixelHeightSize)
{
    // configure the font's size
    // Note: Setting the pixel width (middle argument) to 0 lets FreeType determine font width 
    // based on the provided height.
    // http://learnopengl.com/#!In-Practice/Text-Rendering
    FT_Set_Pixel_Sizes(face, 0, fontPixelHeightSize);

    // save some dereferencing
    FT_GlyphSlot glyph = face->glyph;

    glGenTextures(1, &_textureId);

    // these are some kind of standard texture settings for how to magnify it (detail when 
    // zooming in), "minify" it (detail when zooming out), and set tiling.
    // Note: I don't know how these work or what they do in detail, but they seem to be common
    // in a few texture tutorials.
    glGenSamplers(1, &_textureSamplerId);
    glSamplerParameteri(_textureSamplerId, GL_TEXTURE_WRAP_S, GL_REPEAT);   // "S" is texture X axis
    glSamplerParameteri(_textureSamplerId, GL_TEXTURE_WRAP_T, GL_REPEAT);   // "T" is texture Y axis
    glSamplerParameteri(_textureSamplerId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // "zoom in"
    glSamplerParameteri(_textureSamplerId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // "zoom out"

    // tell the frag shader which texture sampler to use before loading the texture info
    // Note: I tried to explain this to myself, but got lost.  Here's what I was thinking:
    // - Setting an "active texture unit" sets it up to receive new info.
    // - Binding a texture opens a channel between texture loading and texture unit memory.
    // - Binding a sampler tells the "active texture unit" which sampler to use.
    // - Tell the shader which sampler to use.
    // - The sampler-texture unit link will persist even if the "active texture unit" changes.
    // - The sampler number is a signed int, but the sampler ID is OpenGL-generated => unsigned.
    // - If there were only one texture, then the default bindings for sampler 0 and texture 
    // unit 0 would be ok and this could be skipped.  The code is kept here for instruction.
    _textureUnit = 0;
    _textureSamplerNum = 0;
    glActiveTexture(GL_TEXTURE0 + _textureUnit);
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glBindSampler(_textureSamplerNum, _textureSamplerId);
    glUniform1i(_uniformTextSamplerLoc, _textureSamplerNum);



    // before I begin...
    // A texture atlas means that a bunch of different images are loaded into the same texture.  
    // In many applications, such as games with sprites, the textures are all the same size, so 
    // it is rather easy to make a perfect grid.  TrueType fonts are not so pretty for nice 
    // grids, but when rendered properly they look very nice, so I'm willing to jump through 
    // some hoops when loading them.  The characters in TrueType fonts are not the same size 
    // because each character is designed individually and has its own dimensions.  To load 
    // these into an atlas, I am going to run through all the characters and calculate how much 
    // space will be required to load them side-by-side, but be aware that this row of 
    // characters will be of uneven height (not all characters are the same height) and may run 
    // up against the maximum byte width of a texture.  Then I'll allocate the bytes necessary 
    // for a texture to hold all the character's glyphs, and finally I'll load them all 
    // side-by-side in the same order as when I calculated the required texture size.

    // The GL standard defines a maximum size (in bytes) for textures.  This affects 1D and 2D 
    // textures (I've been told that 3D textures have their own max values).  1D is just a line, 
    // so it only applies to that one dimension.  For 2D textures, the max size applies to both 
    // width and height.  This value is determined by the graphics API.  I checked this program
    // (on 3-29-2016), and at this time OpenGL is telling me that my max texture size is 16384 
    // bytes.  This means that I have 16384 bytes for width and 16384 bytes for height, and I 
    // doubt that I am going to max out either with this FreeType library, although if font size (??font size -> bitmap size??)
    // became enormous I might need to start a second row.  But while the GL standard does not 
    // tell the graphics API an upper limit, it does tell them a lower limit for this max value, 
    // which I believe is 1024 bytes.  That is, OpenGL is free to specify that a maximum texture 
    // size is 16kb, or 32kb, or whatever, but the max texture size must not be below 1024 
    // bytes, which should be sufficient to support rather old video or just incapable video 
    // hardware.  A max size of 1024 bytes means that a 2D texture can be, at minimum, 1024x1024 
    // bytes.  
    GLint maxTextureSizeBytes;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSizeBytes);

    // for FreeType fonts under default rendering, 1 pixel == 1 byte
    // Note: FreeType 2 (the header indicates that I am using 2.6.1 as of 3-29-2016) does not 
    // provide RGB data for a texture, but it does provide monochrome (1-bit) or 8-bit greyscale 
    // bitmaps.  
    // http://www.freetype.org/freetype2/docs/ft2faq.html#general-donts
    // Also Note: When loading a glyph, I use FT_LOAD_RENDER, which causes a character's glyph 
    // to be loaded from the TrueType file into a small bitmap in the default render mode.  
    // http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_RENDER
    // Also Also Note: The default render mode loads an 8-bit greyscale bitmap with 256 levels 
    // of opacity (2^8) (implicitly, this means 256 levels of opacity per pixel).  "Use linear 
    // alpha blending and gamma correction to correctly render non-monochrome glyph bitmaps onto 
    // a surface." (alpha blending is activated in the "render text" method) This means that 
    // there is one 8-bit byte per pixel.  Keep this in mind when calculating the texture size.  
    // http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Mode


    // build up the dimensions for the atlas, then make a texture for it later
    // Note: Not only are the dimensions needed before allocating space for the texture, but in 
    // loading up these dimensions BEFORE creating the texture, I can make sure sure that all the
    // characters load (that is, check that FT_Load_Char(...) does not return errors).
    // Also Note: The row width builds additively with each glyph, but the row height is only the 
    // height of the tallest glyph.
    // Also Also Note:??mention max texture size??
    //??why restrict to a max width? trying to mimic the actual bitmap layout??
    unsigned int atlasPixelWidth = 0;
    unsigned int atlasPixelHeight = 0;
    unsigned int rowPixelWidth = 0;
    unsigned int rowPixelHeight = 0;

    // load only the visible characters of the basic ASCII set
    // Note: The basic set is 2^7 = 128 items, and the first 32 are non-printable, so skip them.
    for (size_t charCode = 32; charCode < 128; charCode++)
    {
        if (FT_Load_Char(face, charCode, FT_LOAD_RENDER))
        {
            fprintf(stderr, "Loading character '%c' failed\n", charCode);
            continue;
        }

        // if this glyph would make this row's width exceed the max allowable texture size, 
        // start a new row
        // Note: Remember that FreeType provides an 8-bit alpha bitmap, so in this particular 
        // application, each pixel is 1 byte and pixels and bytes can be compared.
        // Also Note: The "+1" is a 1-pixel "gutter" between characters in case some kind of 
        // float rounding at render time (specifically, calculating texture coordinates S and 
        // T, each by definition on the range [0.0,1.0]) goes 1 pixel further than it should.  
        // This 1-pixel "gutter" prevents that 1 pixel from infringing on the edges of another 
        // glyph.
        // Also Also Note: The unsigned integer cast is to prevent a compiler warning.  Because 
        // I'm picky that way :).
        if ((rowPixelWidth + glyph->bitmap.width + 1) >= (unsigned int)maxTextureSizeBytes)
        {
            // if this row is longer than any previous row, expand the atlas
            // Note: Since this only happens if the next glyph will make the row exceed the max 
            // allowable texture width, the difference between the two will probably only be a 
            // handful of pixels, but these calculations need to be exact to look right.
            atlasPixelWidth = std::max(atlasPixelWidth, rowPixelWidth);

            // row height has already been calculated, so just add it
            atlasPixelHeight += rowPixelHeight;

            // reset row width and height before starting the new row
            rowPixelWidth = 0;
            rowPixelHeight = 0;
        }

        // expand the row's width by the width of this character's glyph + the 1-pixel "gutter"
        rowPixelWidth += glyph->bitmap.width + 1;

        // if the current glyph is taller than any other glyph in the row, accomodate it
        // Note: Yes, this means that there will be wasted bytes, but unless I used some kind of 
        // fancy closest-packing algorithm I am going to have wasted bytes.  But bytes are cheap 
        // (for PCs, at least), so I'm willing to waste some bytes if it means that atlas 
        // loading will be easier.
        rowPixelHeight = std::max(rowPixelHeight, glyph->bitmap.rows);
    }

    // when the above loop exits, it will have adjusted the variables for the last row's width 
    // and height, but not for atlas width and height, so take care of the atlas width and 
    // height the same way as it happens when a new row is created
    atlasPixelWidth = std::max(atlasPixelWidth, rowPixelWidth);
    atlasPixelHeight += rowPixelHeight;


    // allocate space for the texture in GPU memory
    // Note: This is why "atlas width" and "atlas height" had to be computed beforehand.

    // some kind of detail thing; leave at default of 0
    GLint level = 0;

    // tell OpenGL that it should store the data as alpha values (no RGB)
    // Note: That is, it should store [alpha, alpha, alpha, etc.].  If GL_RGBA (red, green, 
    // blue, and alpha) were stated instead, then OpenGL would store the data as 
    // [red, green, blue, alpha, red, green, blue, alpha, red, green, blue, alpha, etc.].
    // Also Note: When writing this program (4-16-2016), GL_ALPHA is a deprecated value to 
    // provideto glTexImage2D(...)'s format (it used to work, but now it doesn't), and the red
    // channel is the only one that allows for a single byte
    // (see https://www.opengl.org/sdk/docs/man/html/glTexImage2D.xhtml), so just shove the 
    // alpha value into the red's byte.
    GLint internalFormat = GL_RED;

    // tell OpenGL that the data is being provided as alpha values
    // Note: Similar to "internal format", but this is telling OpenGL how the data is being
    // provided.  It is possible that many different image file formats store their RGBA 
    // in many different formats, so "provided format" may differ from one file type to 
    // another while "internal format" remains the same in order to provide consistency 
    // after file loading.  In this case though, "internal format" and "provided format"
    // are the same.
    // Also Note: GL_ALPHA is deprecated, so make due with the red byte.
    GLint providedFormat = GL_RED;

    // knowing the provided format is great and all, but OpenGL is getting the data in the 
    // form of a void pointer, so it needs to be told if the data is a singed byte, unsigned 
    // byte, unsigned integer, float, etc.
    // Note: The shader uses values on the range [0.0, 1.0] for color and alpha values, but
    // the FreeType face uses a single byte for each alpha value.  Why?  Because a single 
    // unsigned byte (8 bits) can specify 2^8 = 256 (0 - 255) different values.  This is 
    // good enough for what FreeType is trying to draw.  OpenGL compensates for the byte by
    // dividing it by 256 to get it on the range [0.0, 1.0].  
    // Also Note: Some file formats provide data as sets of floats, in which case this type
    // would be GL_FLOAT.
    GLenum providedFormatDataType = GL_UNSIGNED_BYTE;

    // no border (??units? how does this work? play with it??)
    GLint border = 0;

    // the 0 (null (void *) pointer) at the end tells OpenGL that no data is provided for the 
    // texture right now, so it should only allocate the required memory for now
    glTexImage2D(GL_TEXTURE_2D, level, internalFormat, atlasPixelWidth, atlasPixelHeight,
        border, providedFormat, providedFormatDataType, 0);


    // ??the heck is this??
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // paste all glyph bitmaps into the texture, but when loading them, I need to keep track of 
    // where they are
    int offsetX = 0;
    int offsetY = 0;
    // hijack the "row pixel height" and re-use it for helping to calculate Y offset
    for (size_t charCode = 32; charCode < 128; charCode++)
    {
        if (FT_Load_Char(face, charCode, FT_LOAD_RENDER))
        {
            fprintf(stderr, "Loading character '%c' failed\n", charCode);
            continue;
        }

        // this is the same idea as the "atlas pixel width/height" condition when determining 
        // atlas size, but now it deals with the byte offsets into the loaded texture
        // Note: The unsigned integer cast is to prevent a compiler warning.  Because I'm picky
        // that way :).
        if ((offsetX + glyph->bitmap.width + 1) >= (unsigned int)maxTextureSizeBytes)
        {
            // vertical offset jumps to next row
            offsetY += rowPixelHeight;

            // row height and horizontal offset reset to beginning of row
            rowPixelHeight = 0;
            offsetX = 0;
        }

        // send the glyph's bitmap to its own place in the atlas' texture
        glTexSubImage2D(GL_TEXTURE_2D, level, offsetX, offsetY, glyph->bitmap.width,
            glyph->bitmap.rows, providedFormat, providedFormatDataType, glyph->bitmap.buffer);

        // save glyph info for render time

        // because "advance" (pixel distance to jump before next character that makes the text 
        // appear according to font design) is stored, for reasons only the FreeType creator 
        // knows, in 1/64 pixels, and multiplying by 2^6 (64) will generate it in pixels
        // Note: The Y advance is only used in fonts that are meant to be written vertically.
        // The Y advance does NOT describe the distance between lines of text.  Nevertheless,
        // for the sake of generic font handling, record the Y advance too.
        _glyphCharInfo[charCode].ax = (float)(glyph->advance.x >> 6);
        _glyphCharInfo[charCode].ay = (float)(glyph->advance.y >> 6);

        // pixel distance from font origin (a formally defined bottom-left point for the font 
        // designer) to the bitmap origin (because rectangles outside OpenGL, including the 
        // bitmap standard, define the top left as the rectangle origin) - yay for different 
        // standards mixing it up in the same program
        _glyphCharInfo[charCode].bl = (float)(glyph->bitmap_left);
        _glyphCharInfo[charCode].bt = (float)(glyph->bitmap_top);

        // don't know how FreeType stores glyphs in the TrueType file format and I don't need to 
        // since the "load char" function work, but I do need to know where the glyph's data is 
        // in the atlas' texture
        // Note: Remember that OpenGL defines texture origin as the bottom left of a rectangle.
        // Also Note: Remember that a texture has its own pixel coordinate system S and T that
        // interpolate along a texture from [S=0.0, T=0.0] to [S=1.0T=1.0].
        _glyphCharInfo[charCode].tx = (float)(offsetX / (float)atlasPixelWidth);
        _glyphCharInfo[charCode].ty = (float)(offsetY / (float)atlasPixelHeight);

        // rectangles (including OpenGL textures) are defined by an origin (already recorded) 
        // and width and height
        // Note: The portion of the atlas texture for this character needs an origin and width 
        // and height, and since the origin is in texture coordinates, the width and height for 
        // this portion of the texture must also be in texture coordinates.
        _glyphCharInfo[charCode].bw = (float)(glyph->bitmap.width);
        _glyphCharInfo[charCode].nbw = (float)(glyph->bitmap.width / (float)atlasPixelWidth);
        _glyphCharInfo[charCode].bh = (float)(glyph->bitmap.rows);
        _glyphCharInfo[charCode].nbh = (float)(glyph->bitmap.rows / (float)atlasPixelHeight);

        // just like in the earlier loop
        rowPixelHeight = std::max(rowPixelHeight, glyph->bitmap.rows);
        offsetX += glyph->bitmap.width + 1;
    }

    // create the vertex buffer object (VBO) that will be used to create quads as a base for the 
    // FreeType glyph textures and store the vertex attribtues in a vertex array object (VAO)
    // Note: MUST bind BEFORE setting vertex attribute array pointers or it WILL crash.  The GPU 
    // operates on the values set in the vertex attribute pointers and not on the buffer ID.  
    // The buffer ID is simply a way to tell OpenGL to activate a certain part of the context, 
    // and then the following vertex attribute data tells OpenGL how the next draw call is going 
    // to go down.  This is a GL context thing and those will only work on the currently-set 
    // buffer.  No error is thrown if the current buffer's ID is 0 (no buffer), and that just 
    // begs for a silent bug.  A lot of OpenGL code does not clean up the buffer bindings at the 
    // end of the draw call (why unbind if you're just going to bind another in a moment 
    // anyway?), and in doing so this error might be swallowed.  
    glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &_vboId);

    // bind the vertex array object(VAO) before binding the buffer object so that the vertex 
    // attributes are associated with that buffer.
    glBindVertexArray(_vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, _vboId);

    // 2 floats per screen coord, 2 floats per texture coord, so 1 variable will do
    GLint itemsPerVertexAttrib = 2;

    // how many bytes to "jump" until the next instance of the attribute
    GLint bytesPerVertex = 4 * sizeof(float);

    // this is cast as a pointer due to OpenGL legacy stuff
    GLint bufferStartByteOffset = 0;

    // shorthand for "vertex attribute index"
    GLint vai = 0;

    // screen coordinates first
    // Note: 2 floats starting 0 bytes from set start.
    glEnableVertexAttribArray(vai);
    glVertexAttribPointer(vai, itemsPerVertexAttrib, GL_FLOAT, GL_FALSE, bytesPerVertex,
        (void *)bufferStartByteOffset);

    // texture coordinates second
    // Note: Also floats and still using 2 of them, so the only difference from the screen 
    // coordinates is the offset.
    vai++;
    bufferStartByteOffset += itemsPerVertexAttrib * sizeof(float);
    glEnableVertexAttribArray(vai);
    glVertexAttribPointer(vai, itemsPerVertexAttrib, GL_FLOAT, GL_FALSE, bytesPerVertex,
        (void *)bufferStartByteOffset);

    // cleanup
    // Note: Unbinding order matters for VAO and VBO or else the buffer unbinding will 
    // un-associate it with the vertex attributes.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // no problems initializing atlas (I hope)
    return true;
}

/*------------------------------------------------------------------------------------------------
Description:
    Draws the provided string to the screen with the other arguments dictating details.

    The FreeType shader program MUST be bound before calling.  It is not this class' 
    responsibility to keep track of the shader program.  That is the responsibility of 
    ShaderStorage.
Parameters: 
    str             Self-explanatory.
    posScreenCoord  A 2-float array for the position of the bottom left corner of the first 
                    character in the string.  Values are in screen coordinates (X and Y on the 
                    range [-1,+1].
                    Note: The screen coordinates do not exactly include 1, so to draw in the lower left corner, for example, specify [-0.999f, -0.999f].
    userScale       Font size is dictated in the atlas' initialization, but the textures 
                    themselves can be scaled up.  This may make a more pixelated image, but it 
                    can be a speedy alternative to making a new atlas.
    color           A 4-float array with the order RGBA.
Returns:    None
Creator:    John Cox (4-2016)
------------------------------------------------------------------------------------------------*/
void FreeTypeAtlas::RenderText(const std::string &str, const float posScreenCoord[2],
    const float userScale[2], const float color[4]) const
{
    // the text will be drawn, in part, via a manipulation of pixel alpha values, and apparently
    // OpenGL's blending does this
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // bind the texture that contains the atlas and tell OpenGL 
    glActiveTexture(GL_TEXTURE0 + _textureUnit);
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glBindSampler(_textureSamplerNum, _textureSamplerId);
    glUniform1i(_uniformTextSamplerLoc, _textureSamplerNum);

    // use the user-provided color
    glUniform4fv(_uniformTextColorLoc, 1, color);

    // need to create 1 quad (2 triangles) for each character, each of which occupies a 
    // rectangle in the atlas texture
    glBindBuffer(GL_ARRAY_BUFFER, _vboId);

    // X and Y screen coordinates are on the range [-1,+1]
    float oneOverScreenPixelWidth = 2.0f / glutGet(GLUT_WINDOW_WIDTH);
    float oneOverScreenPixelHeight = 2.0f / glutGet(GLUT_WINDOW_HEIGHT);

    // the glyph origin will advance for each successive character
    // Ex: If the string were "ABC", then "B" draws further right than "A", and "C" further right
    // than "B".
    float glyphOriginX = posScreenCoord[0];
    float glyphOriginY = posScreenCoord[1];

    // set aside memory so that screen coordinate and texture coorninate info can be drawn with a
    // single draw call
    // Note: Each character has 4 points.  See comments on "box" in RenderChar(...) for more 
    // detailed comments.
    std::vector<point> glyphBoxes(4 * str.length());

    // run through each character, gather all the vertex and other info together, and draw it 
    // all in one go
    for (size_t charIndex = 0; charIndex < str.length(); charIndex++)
    {
        char c = str[charIndex];

        // figure out where the texture needs to start drawing in screen coordinates
        // Note: A glyph has a formal origin point that we (humans) usually think of as being where
        // the character "starts".  But as far as OpenGL is concerned, I have a 2D rectangle of pixel
        // data, and that's it.  If I drew that texture at the user-provided x and y, then the 
        // character would likely look like it isn't centered.  The TrueType format provides info 
        // that FreeType extracts so that I can figure out where to draw the texture so that it 
        // LOOKS like the glyph ('g', c, ';', etc.) "starts" at the user-provided x and y.
        float scaledGlyphLeft = _glyphCharInfo[c].bl * oneOverScreenPixelWidth * userScale[0];
        float scaledGlyphWidth = _glyphCharInfo[c].bw * oneOverScreenPixelWidth * userScale[0];
        float scaledGlyphTop = _glyphCharInfo[c].bt * oneOverScreenPixelHeight * userScale[1];
        float scaledGlyphHeight = _glyphCharInfo[c].bh * oneOverScreenPixelHeight * userScale[1];

        // could these be condensed into the "scaled glyph" calulations? yes, but this is clearer to 
        // me
        float screenCoordLeft = glyphOriginX - scaledGlyphLeft;
        float screenCoordRight = screenCoordLeft + scaledGlyphWidth;
        float screenCoordTop = glyphOriginY + scaledGlyphTop;
        float screenCoordBottom = screenCoordTop - scaledGlyphHeight;

        // unlike my project "freeglut_glload_render_freetype", which loads glyphs into their own 
        // textures one at a time (crude, but conveys basics), I can no longer use the whole texture 
        // and must use the offset info that was stored when the atlas was created
        // Note: Remember that textures use their own 2D coordinate system (S,T) to avoid confusion 
        // with screen coordinates (X,Y).
        float sLeft = _glyphCharInfo[c].tx;//0.0f;
        float sRight = _glyphCharInfo[c].tx + _glyphCharInfo[c].nbw;//1.0f;
        float tBottom = _glyphCharInfo[c].ty;
        float tTop = _glyphCharInfo[c].ty + _glyphCharInfo[c].nbh;

        // OpenGL draws triangles, but a rectangle needs to be drawn, so specify the four corners
        // of the box in such a way that GL_LINE_STRIP will draw the two triangle halves of the 
        // box
        // Note: As long as the lines from point A to B to C to D don't cross each other (draw 
        // it on paper), this is fine.  I am going with the pattern 
        // bottom left -> bottom right -> top left -> top right.
        // Also Note: Bitmap standards (and most other rectangle standards in programming, such 
        // as for GUIs) understand the top left as the origin, and therefore the texture's pixels 
        // are provided in a rectangle that goes from top left to bottom right.  OpenGL is the 
        // odd one out in the world of rectangles, and it draws textures from lower left 
        // ([S=0,T=0]) to upper right ([S=1,T=1]).  This means that the texture, from OpenGL's 
        // perspective, is "upside down", hence "t top" being assigned to the bottom screen coordinate and "t bottom" being assigned to the top screen coordinate.  Yay for different standards.
        point box[4] = {
            { screenCoordLeft, screenCoordBottom, sLeft, tTop },
            { screenCoordRight, screenCoordBottom, sRight, tTop },
            { screenCoordLeft, screenCoordTop, sLeft, tBottom },
            { screenCoordRight, screenCoordTop, sRight, tBottom }
        };

        // copy the info on each of the four corners into the array that was created to store 
        // them
        glyphBoxes[(charIndex * 4) + 0] = box[0];
        glyphBoxes[(charIndex * 4) + 1] = box[1];
        glyphBoxes[(charIndex * 4) + 2] = box[2];
        glyphBoxes[(charIndex * 4) + 3] = box[3];

        // advance glyph origin for the next character 
        glyphOriginX += _glyphCharInfo[c].ax * oneOverScreenPixelWidth;
        glyphOriginY += _glyphCharInfo[c].ay * oneOverScreenPixelHeight;
    }

    // load up the data, and only reallocate if you need too
    static unsigned int maxBufferSizeBytes = 0;
    unsigned int numBytes = glyphBoxes.size() * sizeof(point);
    if (numBytes > maxBufferSizeBytes)
    {
        // give a little more space than is necessary
        maxBufferSizeBytes = numBytes + 5;
        glBufferData(GL_ARRAY_BUFFER, maxBufferSizeBytes, glyphBoxes.data(), GL_DYNAMIC_DRAW);
    }
    else
    {
        // buffer already big enough
        glBufferSubData(GL_ARRAY_BUFFER, 0, numBytes, glyphBoxes.data());
    }

    // use these vertex attributes
    glBindVertexArray(_vaoId);

    // all that so that this one function call will work
    // Note: Start at vertex 0 (that is, start at element 0 in the GL_ARRAY_BUFFER) and draw 
    // 4 of them.
    // Also Note: Due to the way that fonts work in texture atlases, only one character is 
    // drawn at a time, which means that only 1 quad is drawn at a time.  Rather than set up 
    // indexes and perform an element draw, just use a GL_TRIANGLE_STRIP.  That works great 
    // for a single (and only a single) quad or for a bunch of quads stacked back to back, which is what I want when drawing text.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, glyphBoxes.size());

    // cleanup
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glBlendFunc(0, 0);
}
