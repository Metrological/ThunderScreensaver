#version 100

attribute vec3  vPosition;
// attribute float vOpacity;

void main()                                
{
    gl_Position = vec4(vPosition, 1.0);
}