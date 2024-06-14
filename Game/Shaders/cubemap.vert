#version 460

#include "globalUBO.glsl"

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 coords;

void main()
{
    coords      = position;
    vec4 pos    = global.proj*vec4(mat3(global.view)*position, 1.0); // not translation of view

    gl_Position.xy = pos.xy;
    gl_Position.z = 0.0;
    gl_Position.w = pos.w;
}
