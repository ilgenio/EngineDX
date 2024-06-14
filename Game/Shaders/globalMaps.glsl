#ifndef _GLOBAL_MAPS_GLSL_
#define _GLOBAL_MAPS_GLSL_

layout(set=0, binding = 0) uniform samplerCube background;
layout(set=0, binding = 1) uniform samplerCube diffuseIBL;
layout(set=0, binding = 2) uniform samplerCube specularIBL;
layout(set=0, binding = 3) uniform sampler2D brdfIBL;

#endif /* _GLOBAL_MAPS_GLSL_ */