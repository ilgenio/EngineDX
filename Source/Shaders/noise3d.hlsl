cbuffer cbPerFrame : register(b0)
{
	float4x4 matVP;
	float4x4 matGeo;
	float uTime;
};

struct PSInput
{
    float3 WorldPos : POSITION;
};


float hash2float(uint x)
{
	return float(x >> 8U) * asfloat(0x33800000U);
}

uint hash(uint x) {

    x = (x ^ (x >> 16)) * 0x21f0aaadU;
    x = (x ^ (x >> 15)) * 0x735a2d97U;
    return x ^ (x >> 15);
}

uint hash(uint2 x) 
{ 
    return hash(x.x ^ hash(x.y));
}  // for 2D input

uint hash(uint3 x) 
{ 
    return hash(x.x ^ hash(x.yz)); 
} // for 3D input


// bilinear interpolation
float blerp(float va, float vb, float vc, float vd, float2 u)
{
	return lerp(lerp(va, vb, u.x), lerp(vc, vd, u.x), u.y);
}

const float TAU = 6.283185307179586;

float3 grad(uint3 x) { 
    uint h0 = hash(x);
    uint h1 = hash(h0);
    float c = 2.0*hash2float(h0) - 1.0,     // c = cos(theta) = cos(acos(2x-1)) = 2x-1
          s = sqrt(1.0 - c*c);              // s = sin(theta) = sin(acos(c)) = sqrt(1-c*c)
    float phi = TAU * hash2float(h1);       // use the 2nd random for the azimuth (longitude)
    return float3(cos(phi) * s, sin(phi) * s, c);
}

// trilinear interpolation
float tlerp(float v0, float v1, float v2, float v3, float v4, 
            float v5, float v6, float v7, float3 u)
{
    return lerp(blerp(v0, v1, v2, v3, u.xy), blerp(v4, v5, v6, v7, u.xy), u.z);
}

float gradientnoise(float3 p) {
    float3 i = floor(p);
    float3 g0 = grad(i);
    float3 g1 = grad(i + float3(1.0, 0.0, 0.0));
    float3 g2 = grad(i + float3(0.0, 1.0, 0.0));
    float3 g3 = grad(i + float3(1.0, 1.0, 0.0));
    float3 g4 = grad(i + float3(0.0, 0.0, 1.0));
    float3 g5 = grad(i + float3(1.0, 0.0, 1.0));
    float3 g6 = grad(i + float3(0.0, 1.0, 1.0));
    float3 g7 = grad(i + float3(1.0, 1.0, 1.0));

    float3 f = frac(p);
    float v0 = dot(g0, f - float3(0.0, 0.0, 0.0));
    float v1 = dot(g1, f - float3(1.0, 0.0, 0.0));
    float v2 = dot(g2, f - float3(0.0, 1.0, 0.0));
    float v3 = dot(g3, f - float3(1.0, 1.0, 0.0));
    float v4 = dot(g4, f - float3(0.0, 0.0, 1.0));
    float v5 = dot(g5, f - float3(1.0, 0.0, 1.0));
    float v6 = dot(g6, f - float3(0.0, 1.0, 1.0));
    float v7 = dot(g7, f - float3(1.0, 1.0, 1.0));

   // quintic interpolation
    float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    return tlerp(v0, v1, v2, v3, v4, v5, v6, v7, u);
}

#define OCTAVES 8  // Number of octaves (adjust as needed)

float fbm(float3 st)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.1;

    for (int i = 0; i < OCTAVES; i++)
    {
        value += amplitude * (gradientnoise(st * frequency)+0.5);
        
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}


float4 main(PSInput pin) : SV_TARGET
{
    float res = fbm(pin.WorldPos*24.0+uTime);
    
	return float4(res, res, res, 1.0);
}
