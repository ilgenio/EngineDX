cbuffer MipChainParams : register(b0)
{
    uint2 DstDim;
};

Texture2D<float4> SrcMipLevel : register(t0);
RWTexture2D<float4> DstMipLevel : register(u0);

[numthreads(8, 8, 1)]
void mipChainCS(uint2 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= DstDim.x || DTid.y >= DstDim.y)
        return;

    // Compute the coordinates of the 2x2 block in the source mip level
    uint2 srcCoord = DTid * 2;

    // Load the four texels from the source mip level
    float4 c0 = SrcMipLevel.Load(int3(srcCoord, 0));
    float4 c1 = SrcMipLevel.Load(int3(srcCoord + uint2(1, 0), 0));
    float4 c2 = SrcMipLevel.Load(int3(srcCoord + uint2(0, 1), 0));
    float4 c3 = SrcMipLevel.Load(int3(srcCoord + uint2(1, 1), 0));

    // Average the four texels
    float4 avgColor = (c0 + c1 + c2 + c3) * 0.25;

    // Store the result in the destination mip level
    DstMipLevel[DTid] = avgColor;
}