#pragma once

#include "ThirdParty/glload/include/glload/gl_4_4.h"

void APIENTRY DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const GLvoid* userParam);