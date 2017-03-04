#version 440

layout (location = 0) in vec2 screenCoord;
layout (location = 1) in vec2 textureCoord;    

// output to frag shader
// Note: This is the interpolated position between coordinates.
smooth out vec2 texturePos; 

void main(void) {
    // gl_Position is defined as a vec4
    // Note: See https://www.opengl.org/sdk/docs/man/html/gl_Position.xhtml
    // Also Note: I want the text to draw on top of everything else, so give it a Z coordinate 
    // of 0 (a screen coordinate of Z < 0 will be "behind" the camera and therefore wouldn't 
    // draw).
    // Also Also Note: Nothing is being transformed here, so W's value doesn't matter, but W=1 
    // is pretty common, so I'll go with it.
    gl_Position = vec4(screenCoord, 0, 1);

    // this is a texture coordinate for one of the three corners of the triangle 
    texturePos = textureCoord;
}
