#include "Include/OpenGlErrorHandling.h"

#include <string>
#include <stdio.h>

/*------------------------------------------------------------------------------------------------
Description:
    Rather than calling glGetError(...) every time I make an OpenGL call, I register this
    function as the debug callback.  If an error or any OpenGL message in general pops up, this
    prints it to stderr.  I can turn it on and off by enabling and disabling the "#define DEBUG" 
    statement in main(...).

    The function type (CODEGEN_FUNCPTR) must be the same function type as other OpenGL 
    functions.  For Windows, that is APIENTRY.  For Linux, it isn't.  I don't know more than 
    that.
Parameters:
    Unknown.  The function pointer is provided to glDebugMessageCallbackARB(...), and that
    function is responsible for calling this one as it sees fit.
Returns:    None
Creator:    John Cox (2014)
------------------------------------------------------------------------------------------------*/
void CODEGEN_FUNCPTR DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const GLvoid* userParam)
{
    std::string srcName;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API: srcName = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: srcName = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: srcName = "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: srcName = "Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION: srcName = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER: srcName = "Other"; break;
    default:
        srcName = "UNKNOWN SOURCE";
        break;
    }

    std::string errorType;
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: errorType = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: errorType = "Deprecated Functionality"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: errorType = "Undefined Behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY: errorType = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: errorType = "Performance"; break;
    case GL_DEBUG_TYPE_OTHER: errorType = "Other"; break;
    default:
        errorType = "UNKNOWN ERROR TYPE";
        break;
    }

    std::string typeSeverity;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: typeSeverity = "High"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: typeSeverity = "Medium"; break;
    case GL_DEBUG_SEVERITY_LOW: typeSeverity = "Low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: typeSeverity = "Notification"; break;
    default:
        typeSeverity = "UNKNOWN SEVERITY";
        break;
    }

    //??the heck is id??
    fprintf(stderr, "DebugFunc: length = '%d', id = '%u', userParam = '%x'\n", length, id, (unsigned int)userParam);
    fprintf(stderr, "%s from %s,\t%s priority\nMessage: %s\n",
        errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message);
    fprintf(stderr, "\n");  // separate this error from the next thing that prints
}

//
//GLenum err = glGetError();
//if (GL_NO_ERROR != err)
//{
//    switch (err)
//    {
//    case GL_INVALID_ENUM:
//        printf("GL error: GL_INVALID_ENUM\n");
//        break;
//    case GL_INVALID_VALUE:
//        printf("GL error: GL_INVALID_VALUE\n");
//        break;
//    case GL_INVALID_OPERATION:
//        printf("GL error: GL_INVALID_OPERATION\n");
//        break;
//    case GL_STACK_OVERFLOW:
//        printf("GL error: GL_STACK_OVERFLOW\n");
//        break;
//    case GL_STACK_UNDERFLOW:
//        printf("GL error: GL_STACK_UNDERFLOW\n");
//        break;
//    case GL_OUT_OF_MEMORY:
//        printf("GL error: GL_OUT_OF_MEMORY\n");
//        break;
//    case GL_INVALID_FRAMEBUFFER_OPERATION:
//        printf("GL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
//        break;
//        //case GL_CONTEXT_LOST: // OpenGL 4.5 or higher
//        //    break;
//    case GL_TABLE_TOO_LARGE:
//        printf("GL error: GL_TABLE_TOO_LARGE\n");
//        break;
//    default:
//        printf("GL error: UNKNOWN\n");
//        break;
//    }
//}
