#include "Globals.h"

#include "CubemapMesh.h"

#include "Application.h"
#include "ModuleResources.h"

namespace
{

    float vertices[6 * 6 * 3] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
}

CubemapMesh::CubemapMesh()
{
    front[0] = {1.0f, 0.0f, 0.0f};
    front[1] = {-1.0f, 0.0f, 0.0f};
    front[2] = {0.0f, 1.0f, 0.0f};
    front[3] = {0.0f, -1.0f, 0.0f};
    front[4] = {0.0f, 0.0f, -1.0f};
    front[5] = {0.0f, 0.0f, 1.0f};

    up[0] = {0.0f, 1.0f, 0.0f};
    up[1] = {0.0f, 1.0f, 0.0f};
    up[2] = {0.0f, 0.0f, 1.0f};
    up[3] = {0.0f, 0.0f, -1.0f};
    up[4] = {0.0f, 1.0f, 0.0f};
    up[5] = {0.0f, 1.0f, 0.0f};

    inputLayout     = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    inputLayoutDesc = { &inputLayout, 1 };
    vertexBuffer = app->getResources()->createDefaultBuffer(&vertices[0], sizeof(vertices), "CubemapMesh");

    vertexBufferView = { vertexBuffer->GetGPUVirtualAddress(), sizeof(vertices), sizeof(float)*3};
}

CubemapMesh::~CubemapMesh()
{
}

void CubemapMesh::draw(ID3D12GraphicsCommandList* commandList) const
{
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(6 * 6, 1, 0, 0);
}

