#pragma once

class CubemapMesh;

// SkyboxRenderPass encapsulates the rendering of a skybox using a cubemap texture in a DirectX 12 application.
// It manages the root signature, pipeline state object, and mesh resources required for skybox rendering.
// The class provides methods to record draw commands for the skybox, handling shader setup and resource binding.
// Use this class to efficiently render a background environment with a cubemap in your graphics pipeline.
class SkyboxRenderPass
{
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;

    std::unique_ptr<CubemapMesh> cubemapMesh;
public:
    SkyboxRenderPass();
    ~SkyboxRenderPass();

    void record(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE cubemapSRV, const Quaternion& cameraRot, const Matrix& projection);

private:
    struct SkyParams
    {
        Matrix vp;
        BOOL   flipX;
        BOOL   flipZ;
        UINT   padding[2];
    };

    bool createRootSignature();
    bool createPSO();
};