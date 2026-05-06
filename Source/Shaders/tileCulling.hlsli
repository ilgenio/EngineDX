#ifndef TILECULLING_HLSLI
#define TILECULLING_HLSLI

#define TILE_RES 16
#define MAX_LIGHTS_PER_TILE 64

uint getNumTiles(uint size)
{
    return ((size + TILE_RES - 1) / TILE_RES);
}

uint2 getNumTiles(uint width, uint height)
{
    return uint2(getNumTiles(width), getNumTiles(height));
}

uint getTileIndex(uint x, uint y, uint width)
{
    uint tileX = x / TILE_RES;
    uint tileY = y / TILE_RES;

    return tileY*getNumTiles(width) + tileX;
}

uint getTileIndexFromNDC(float2 ndcPos, uint width, uint height)
{
    uint x = uint(saturate(ndcPos.x * 0.5 + 0.5) * width) / TILE_RES;
    uint y = uint((1.0 - saturate(ndcPos.y * 0.5 + 0.5)) * height) / TILE_RES;

    return getTileIndex(x, y, width);
}

#endif // TILECULLING_HLSLI