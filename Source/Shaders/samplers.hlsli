#ifndef _SAMPLERS_HLSLI_
#define _SAMPLERS_HLSLI_

SamplerState bilinearWrap : register(s0);
SamplerState pointWrap : register(s1);
SamplerState bilinearClamp : register(s2);
SamplerState pointClamp : register(s3); 

#endif // _SAMPLERS_HLSLI_