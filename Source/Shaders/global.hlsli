#ifndef _GLOBAL_UBO_GLSL_
#define _GLOBAL_UBO_GLSL_

// Flags
#define DEBUG_DISABLE_IRIDESCENCE 1

// Render

#define DEBUG_RENDER_NONE 0
#define DEBUG_RENDER_IRIDESCENCE_FRESNEL 1


// Global constants
cbuffer GlobalUBO : register(b0)
{
	float4x4 view;
	float4x4 proj;
	float4x4 viewProj;
    float4   viewPos;

    float4   lightDir;
    float4   lightColor;
    float4   ambientLightColor;
    int      debugFlags;
    int      debugRender;
    int      padding0;
    int      padding1;
};

#endif /* _GLOBAL_UBO_GLSL_ */