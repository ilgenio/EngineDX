float2 main(float4 position : SV_POSITION) : SV_TARGET
{
    float depth = (position.z / position.w);
    depth = exp2(depth * 16.0); // Exponential depth to improve precision for shadow mapping

    return float2(depth, depth*depth);
}