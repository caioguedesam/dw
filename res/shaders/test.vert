#version 460 core

layout(location = 0) out vec2 voTexCoord;

void main()
{
    voTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(voTexCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}
