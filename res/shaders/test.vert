#version 460 core

layout(location = 0) in vec3 viPos;

void main()
{
    gl_Position = vec4(viPos, 1.0);
}
