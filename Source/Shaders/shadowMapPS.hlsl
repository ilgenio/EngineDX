float2 main(float4 position : SV_POSITION) : SV_TARGET
{
    float depth = position.z / position.w; 

    return float2(depth, depth*depth);
}