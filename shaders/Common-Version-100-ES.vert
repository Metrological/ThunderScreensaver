#version 100 es

attribute vec3  vPosition;
attribute float vOpacity;

void main()                                
{
    gl_Position = vec4(vPosition, vOpacity);
}