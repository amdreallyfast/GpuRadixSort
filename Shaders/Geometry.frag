#version 440

// because gl_FragColor was apparently deprecated as of version 120 (currently using 440)
// Note: If I set gl_FragColor to an intermediate vec4, then the "glFragColor is deprecated"
// compiler error goes away.  I'd rather not rely on this though, so I will follow good practice
// and define my own.
out vec4 finalFragColor;

void main()
{
    // just make the shapes white in this demo
    finalFragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
