#include "Globals.h"
#include "SphereMesh.h"

#include "Application.h"
#include "ModuleResources.h"


#define PAR_SHAPES_IMPLEMENTATION
#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif 
#pragma warning(push)
#pragma warning(disable : 4305) 
#pragma warning(disable : 4244) 
#include "par_shapes.h"
#pragma warning(pop)

SphereMesh::SphereMesh(int slices, int stacks)
{
    par_shapes_mesh* mesh = par_shapes_create_parametric_sphere(slices, stacks);

    struct MeshVertex
    {
        Vector3 position;
        Vector2 texCoord;
        Vector3 normal;
    };

    std::vector<MeshVertex> vertices;
    vertices.reserve(mesh->npoints);

    // Create buffers
    for(int i=0; i< mesh->npoints; ++i)
    {
        float* p = &mesh->points[i * 3];
        float* n = &mesh->normals[i * 3];
        float* t = &mesh->tcoords[i * 2];

        Vector3 pos(p[0], p[1], p[2]);
        Vector2 tex(t[0], t[1]);
        Vector3 norm(n[0], n[1], n[2]);

        norm.Normalize();

        vertices.push_back({ pos, tex, norm });
    }

    numVertices = mesh->npoints;
    numIndices = mesh->ntriangles * 3;;

    inputLayout[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    inputLayout[1] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    inputLayout[2] = {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

    inputLayoutDesc = { inputLayout, 3 };

    vertexBuffer = app->getResources()->createDefaultBuffer(&vertices[0], sizeof(MeshVertex) * numVertices, "SphereMesh");
    indexBuffer = app->getResources()->createDefaultBuffer(&mesh->triangles, sizeof(PAR_SHAPES_T) * numIndices, "SphereMesh");

    vertexBufferView = { vertexBuffer->GetGPUVirtualAddress(), sizeof(MeshVertex) * numVertices, sizeof(MeshVertex) };
    indexBufferView = { indexBuffer->GetGPUVirtualAddress(), sizeof(PAR_SHAPES_T) * numIndices, sizeof(PAR_SHAPES_T) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT };

    par_shapes_free_mesh(mesh);
}

SphereMesh::~SphereMesh()
{
}

void SphereMesh::draw(ID3D12GraphicsCommandList* commandList) const
{
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
}