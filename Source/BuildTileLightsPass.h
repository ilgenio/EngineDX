#pragma once

#include <span>

struct Point;
struct Spot;

class BuildTileLightsPass
{
    enum RootParams
    {
        ROOTPARAM_CONSTANTS = 0,
        ROOTPARAM_DEPTH_TABLE,
        ROOTPARAM_POINT_SPHERE_SRV,
        ROOTPARAM_SPOT_SPHERE_SRV,
        ROOTPARAM_POINT_LIST_UAV,
        ROOTPARAM_SPOT_LIST_UAV,
        ROOTPARAM_COUNT
    };

    struct Constants
    {
        Matrix view;
        Matrix proj;

        int width;
        int height;

        int numPointLights;
        int numSpotLights;
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;

    std::vector<Vector4> pointSphereData;
    std::vector<Vector4> spotSphereData;

    ComPtr<ID3D12Resource> pointLists[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12Resource> spotLists[FRAMES_IN_FLIGHT];
    UINT width = 0;
    UINT height = 0;

public:
    BuildTileLightsPass();
    ~BuildTileLightsPass();

    void record(ID3D12GraphicsCommandList* commandList, int width, int height, const Matrix& view, const Matrix& proj,
                std::span<Point*> pointLights, std::span<Spot*> spotLights, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV);

    void resize(UINT width, UINT height);

    D3D12_GPU_VIRTUAL_ADDRESS getPointListAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS getSpotListAddress() const;

private:

    bool createRootSignature();
    bool createPSO();
    void generateSphereData(std::span<Point*> pointLights, std::span<Spot*> spotLights);
};