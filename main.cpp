// the OpenGL version include also includes all previous versions
// Build note: Due to a minefield of preprocessor build flags, the gl_load.hpp must come after 
// the version include.
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
// Build note: Also need to link opengl32.lib (unknown directory, but VS seems to know where it 
// is, so don't put in an "Additional Library Directories" entry for it).
// Build note: Also need to link glload/lib/glloadD.lib.
#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "ThirdParty/glload/include/glload/gl_load.hpp"

// Build note: Must be included after OpenGL code (in this case, glload).
// Build note: Also need to link freeglut/lib/freeglutD.lib.  However, the linker will try to 
// find "freeglut.lib" (note the lack of "D") instead unless the following preprocessor 
// directives are set either here or in the source-building command line (VS has a
// "Preprocessor" section under "C/C++" for preprocessor definitions).
// Build note: Also need to link winmm.lib (VS seems to know where it is, so don't put in an 
// "Additional Library Directories" entry).
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "ThirdParty/freeglut/include/GL/freeglut.h"

// this linking approach is very useful for portable, crude, barebones demo code, but it is 
// better to link through the project building properties
#pragma comment(lib, "ThirdParty/glload/lib/glloadD.lib")
#pragma comment(lib, "opengl32.lib")            // needed for glload::LoadFunctions()
#pragma comment(lib, "ThirdParty/freeglut/lib/freeglutD.lib")
#ifdef WIN32
#pragma comment(lib, "winmm.lib")               // Windows-specific; freeglut needs it
#endif

// apparently the FreeType lib also needs a companion file, "freetype261d.pdb"
#pragma comment (lib, "ThirdParty/freetype-2.6.1/objs/vc2010/Win32/freetype261d.lib")

#include <stdio.h>
#include <memory>
#include <algorithm>    // for generating demo data

// for basic OpenGL stuff
#include "Include/OpenGlErrorHandling.h"
#include "Shaders/ShaderStorage.h"

// for particles, where they live, and how to update them
#include "ThirdParty/glm/vec2.hpp"

#include "Include/SSBOs/originalData.h"
#include "Include/SSBOs/OriginalData.h" // for generating demo data
#include "Include/ComputeControllers/ParallelSort.h"

// for the frame rate counter
#include "Include/RenderFrameRate/FreeTypeEncapsulated.h"
#include "Include/RenderFrameRate/Stopwatch.h"

Stopwatch gTimer;
FreeTypeEncapsulated gTextAtlases;

OriginalDataSsbo::UNIQUE_PTR originalData = nullptr;
std::unique_ptr<ParallelSort> parallelSort = nullptr;

const unsigned int MAX_DATA_COUNT = 1000000;

/*------------------------------------------------------------------------------------------------
Description:
    Governs window creation, the initial OpenGL configuration (face culling, depth mask, even
    though this is a 2D demo and that stuff won't be of concern), the creation of geometry, and
    the creation of a texture.
Parameters: None
Returns:    None
Creator:    John Cox (3-7-2016)
------------------------------------------------------------------------------------------------*/
void Init()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    // inactive particle Z = -0.6   alpha = 0
    // active particle Z = -0.7     alpha = 1
    // polygon fragment Z = -0.8    alpha = 1
    // Note: The blend function is done via glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).  
    // The first argument is the scale factor for the source color (presumably the existing 
    // fragment color in the frame buffer) and the second argument is the scale factor for the 
    // destination color (presumably the color of the fragment that is being added).  The second 
    // argument ("one minus source alpha") means that, when any color is being added, the 
    // resulting color will be "(existingFragmentAlpha * existingFragmentColor) - 
    // (addedFragmentAlpha * addedFragmentColor)".  
    // Also Note: If the color furthest from the camera is black (vec4(0,0,0,0)), then any 
    // color on top of it will end up as (using the equation) "vec4(0,0,0,0) - whatever", which 
    // is clamped at 0.  So put the opaque (alpha=1) furthest from the camera (this demo is 2D, 
    // so make it a lower Z).  The depth range is 0-1, so the lower Z limit is -1.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    // FreeType initialization
    std::string freeTypeShaderKey = "freetype";
    shaderStorageRef.NewShader(freeTypeShaderKey);
    shaderStorageRef.AddAndCompileShaderFile(freeTypeShaderKey, "Shaders/FreeType.vert", GL_VERTEX_SHADER);
    shaderStorageRef.AddAndCompileShaderFile(freeTypeShaderKey, "Shaders/FreeType.frag", GL_FRAGMENT_SHADER);
    shaderStorageRef.LinkShader(freeTypeShaderKey);
    GLuint freeTypeProgramId = shaderStorageRef.GetShaderProgram(freeTypeShaderKey);
    gTextAtlases.Init("ThirdParty/freetype-2.6.1/FreeSans.ttf", freeTypeProgramId);

    // Note: Compute headers with #define'd buffer binding locations makes it easy for the 
    // ParallelSort compute controller's shaders to access originalData's data without 
    // needing to pass the SSBO into it.  GPU computing in multiple steps creates coupling 
    // between the SSBOs and the shaders, but the compute headers lessen the coupling that needs 
    // to happen on the CPU side.
    originalData = std::make_unique<OriginalDataSsbo>(MAX_DATA_COUNT);

    // generate dummy data for this demo
    std::vector<OriginalData> demoData(originalData->NumItems());
    //demoData[0]._value = 12;
    //demoData[1]._value = 1;
    //demoData[2]._value = 9;
    //demoData[3]._value = 2;
    //demoData[4]._value = 0;
    //demoData[5]._value = 11;
    //demoData[6]._value = 7;
    //demoData[7]._value = 3;
    //demoData[8]._value = 4;
    //demoData[9]._value = 15;
    //demoData[10]._value = 8;
    //demoData[11]._value = 5;
    //demoData[12]._value = 14;
    //demoData[13]._value = 13;
    //demoData[14]._value = 10;
    //demoData[15]._value = 6;
    for (size_t dataIndex = 0; dataIndex < demoData.size(); dataIndex++)
    {
        demoData[dataIndex]._value = dataIndex;
    }
    
    // scramble it
    std::random_shuffle(demoData.begin(), demoData.end());

    // upload the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, originalData->BufferId());
    unsigned int bufferSizeBytes = demoData.size() * sizeof(OriginalData);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, demoData.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);



    parallelSort = std::make_unique<ParallelSort>(originalData);
    parallelSort->Sort();
    parallelSort->Sort();


    printf("");

    // the timer will be used for framerate calculations
    gTimer.Start();
}

/*------------------------------------------------------------------------------------------------
Description:
    Updates particle positions, generates the quad tree for the particles' new positions, and 
    commands a new draw.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (1-2-2017)
------------------------------------------------------------------------------------------------*/
void UpdateAllTheThings()
{
    // just hard-code it for this demo
    float deltaTimeSec = 0.01f;

    // tell glut to call this display() function again on the next iteration of the main loop
    // Note: https://www.opengl.org/discussion_boards/showthread.php/168717-I-dont-understand-what-glutPostRedisplay()-does
    // Also Note: This display() function will also be registered to run if the window is moved
    // or if the viewport is resized.  If glutPostRedisplay() is not called, then as long as the
    // window stays put and doesn't resize, display() won't be called again (tested with 
    // debugging).
    // Also Also Note: It doesn't matter where this is called in this function.  It sets a flag
    // for glut's main loop and doesn't actually call the registered display function, but I 
    // got into the habbit of calling it at the end.
    glutPostRedisplay();
}

/*------------------------------------------------------------------------------------------------
Description:
    This is the rendering function.  It tells OpenGL to clear out some color and depth buffers,
    to set up the data to draw, to draw than stuff, and to report any errors that it came across.
    This is not a user-called function.

    This function is registered with glutDisplayFunc(...) during glut's initialization.
Parameters: None
Returns:    None
Creator:    John Cox (2-13-2016)
------------------------------------------------------------------------------------------------*/
void Display()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the frame rate once per second in the lower left corner
    GLfloat color[4] = { 0.5f, 0.5f, 0.0f, 1.0f };
    char str[32];
    static int elapsedFramesPerSecond = 0;
    static double elapsedTime = 0.0;
    static double frameRate = 0.0;
    elapsedFramesPerSecond++;
    elapsedTime += gTimer.Lap();
    if (elapsedTime > 1.0f)
    { 
        frameRate = (double)elapsedFramesPerSecond / elapsedTime;
        elapsedFramesPerSecond = 0;
        elapsedTime -= 1.0f;
    }
    sprintf(str, "%.2lf", frameRate);

    // Note: The font textures' orgin is their lower left corner, so the "lower left" in screen 
    // space is just above [-1.0f, -1.0f].
    float xy[2] = { -0.99f, -0.99f };
    float scaleXY[2] = { 1.0f, 1.0f };

    // the first time that "get shader program" runs, it will load the atlas
    glUseProgram(ShaderStorage::GetInstance().GetShaderProgram("freetype"));
    gTextAtlases.GetAtlas(48)->RenderText(str, xy, scaleXY, color);

    // clean up bindings
    glUseProgram(0);
    glBindVertexArray(0);       // unbind this BEFORE the buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // tell the GPU to swap out the displayed buffer with the one that was just rendered
    glutSwapBuffers();
}

/*------------------------------------------------------------------------------------------------
Description:
    Tell's OpenGL to resize the viewport based on the arguments provided.  This is an 
    opportunity to call glViewport or glScissor to keep up with the change in size.
    
    This is not a user-called function.  It is registered with glutReshapeFunc(...) during 
    glut's initialization.
Parameters:
    w   The width of the window in pixels.
    h   The height of the window in pixels.
Returns:    None
Creator:    John Cox (2-13-2016)
------------------------------------------------------------------------------------------------*/
void Reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

/*------------------------------------------------------------------------------------------------
Description:
    Executes when the user presses a key on the keyboard.

    This is not a user-called function.  It is registered with glutKeyboardFunc(...) during
    glut's initialization.

    Note: Although the x and y arguments are for the mouse's current position, this function does
    not respond to mouse presses.
Parameters:
    key     The ASCII code of the key that was pressed (ex: ESC key is 27)
    x       The horizontal viewport coordinates of the mouse's current position.
    y       The vertical window coordinates of the mouse's current position
Returns:    None
Creator:    John Cox (2-13-2016)
------------------------------------------------------------------------------------------------*/
void Keyboard(unsigned char key, int x, int y)
{
    // this statement is mostly to get ride of an "unreferenced parameter" warning
    printf("keyboard: x = %d, y = %d\n", x, y);
    switch (key)
    {
    case 27:
    {
        // ESC key
        glutLeaveMainLoop();
        return;
    }
    default:
        break;
    }
}

/*------------------------------------------------------------------------------------------------
Description:
    I don't know what this does, but I've kept it around since early times, and this was the 
    comment given with it:
    
    "Called before FreeGLUT is initialized. It should return the FreeGLUT display mode flags 
    that you want to use. The initial value are the standard ones used by the framework. You can 
    modify it or just return you own set.  This function can also set the width/height of the 
    window. The initial value of these variables is the default, but you can change it."
Parameters:
    displayMode     ??
    width           ??
    height          ??
Returns:
    ??what??
Creator:    John Cox (2-13-2016)
------------------------------------------------------------------------------------------------*/
unsigned int Defaults(unsigned int displayMode, int &width, int &height) 
{
    // this statement is mostly to get ride of an "unreferenced parameter" warning
    printf("Defaults: width = %d, height = %d\n", width, height);
    return displayMode; 
}

/*------------------------------------------------------------------------------------------------
Description:
    Cleans up GPU memory.  This might happen when the processes die, but be a good memory steward
    and clean up properly.

    Note: A big program would have the textures, program IDs, buffers, and other things 
    encapsulated somewhere, and each other those would do the cleanup, but in this barebones 
    demo, I can just clean up everything here.
Parameters: None
Returns:    None
Creator:    John Cox (2-13-2016)
------------------------------------------------------------------------------------------------*/
void CleanupAll()
{
}

/*------------------------------------------------------------------------------------------------
Description:
    Program start and end.
Parameters:
    argc    The number of strings in argv.
    argv    A pointer to an array of null-terminated, C-style strings.
Returns:
    0 if program ended well, which it always does or it crashes outright, so returning 0 is fine
Creator:    John Cox (2-13-2016)
------------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    glutInit(&argc, argv);

    int width = 500;
    int height = 500;
    unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
    displayMode = Defaults(displayMode, width, height);

    glutInitDisplayMode(displayMode);
    glutInitContextVersion(4, 4);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // enable this for automatic message reporting (see OpenGlErrorHandling.cpp)
//#define DEBUG
#ifdef DEBUG
    glutInitContextFlags(GLUT_DEBUG);
#endif

    glutInitWindowSize(width, height);
    glutInitWindowPosition(300, 200);
    int window = glutCreateWindow(argv[0]);

    glload::LoadTest glLoadGood = glload::LoadFunctions();
    // ??check return value??

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    if (!glload::IsVersionGEQ(3, 3))
    {
        printf("Your OpenGL version is %i, %i. You must have at least OpenGL 3.3 to run this tutorial.\n",
            glload::GetMajorVersion(), glload::GetMinorVersion());
        glutDestroyWindow(window);
        return 0;
    }

    if (glext_ARB_debug_output)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallback(DebugFunc, (void*)15);
    }

    Init();

    glutIdleFunc(UpdateAllTheThings);
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMainLoop();

    CleanupAll();

    return 0;
}