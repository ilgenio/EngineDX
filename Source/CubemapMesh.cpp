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
Vector3 CubemapMesh::front[6] = {
    {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}
};

Vector3 CubemapMesh::up[6] = {
    {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
    {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
};

const D3D12_INPUT_ELEMENT_DESC CubemapMesh::inputLayout[CubemapMesh::numVertexAttribs] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };
const D3D12_INPUT_LAYOUT_DESC CubemapMesh::inputLayoutDesc = { &CubemapMesh::inputLayout[0], UINT(std::size(CubemapMesh::inputLayout)) };

CubemapMesh::CubemapMesh()
{
    vertexBuffer = app->getResources()->createBuffer(&vertices[0], sizeof(vertices), "CubemapMesh");
}

CubemapMesh::~CubemapMesh()
{
}
