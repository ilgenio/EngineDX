#pragma once

#include<span>

struct RenderMesh;
struct RenderData;

class ShadowMapPass
{
    ComPtr<ID3D12Resource> shadowMap;

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
    Matrix viewProj = Matrix::Identity;

public:
    ShadowMapPass();
    ~ShadowMapPass();

    void buildFrustum(Vector4 planes[6], const Vector3& lightDir, const Vector4& sphereBound);
    Matrix getViewProj() const { return viewProj; }

    void render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData);

private:
    bool createRootSignature();
    bool createPSO();
    bool createShadowMapResource();

};