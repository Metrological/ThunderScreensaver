#version 300 es

//const input variables from the user implementation
layout(location = 0) in vec3 vPosition;

void main()
{
    gl_Position = vec4(vPosition, 1.0);
}
