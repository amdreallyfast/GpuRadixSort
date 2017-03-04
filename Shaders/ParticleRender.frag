#version 440

smooth in vec4 particleColor;

// because gl_FragColor was apparently deprecated as of version 120 (currently using 440)
// Note: If I set gl_FragColor to an intermediate vec4, then the "glFragColor is deprecated"
// compiler error goes away.  I'd rather not rely on this though, so I will follow good practice
// and define my own.
out vec4 finalFragColor;

void main()
{
    finalFragColor = particleColor;
}