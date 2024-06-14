#version 460 

#include "common.glsl"
#include "toneMap.glsl"
#include "globalMaps.glsl"

layout (location = 0) out vec4 outFragColor;
layout(location = 0) in vec3 coords;

void main()
{
    outFragColor.rgb = toneMap(texture(background, coords).rgb);
}
