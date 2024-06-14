#version 460 

#include "common.glsl"

layout(location = 0) in vec3 coords;

layout (location = 0) out vec4 outFragColor;

layout(set=0, binding = 0) uniform sampler2D skybox;

vec2 cartesianToEquirectangular(in vec3 dir)
{
    float phi;

    phi = atan(dir.z, dir.x); // between -PI , PI
    phi = 1.0-(phi/(2.0*PI)+0.5);

    float theta = asin(dir.y);  // between -PI/ ,  PI/2
    theta = 1.0-(theta/PI+0.5);

    return vec2(phi, theta);
}

void main()
{
    outFragColor = texture(skybox, cartesianToEquirectangular(normalize(coords)));
}

