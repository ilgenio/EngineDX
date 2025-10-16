#include "Globals.h"
#include "BasicMesh.h"

#include "Application.h"
#include "ModuleResources.h"

#include "gltf_utils.h"

#include "mikktspace.h"
#include "weldmesh.h"

const D3D12_INPUT_ELEMENT_DESC BasicMesh::inputLayout[numVertexAttribs] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texCoord0), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                            {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                            {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

const D3D12_INPUT_LAYOUT_DESC BasicMesh::inputLayoutDesc = { &inputLayout[0], UINT(std::size(inputLayout)) };

BasicMesh::BasicMesh()
{
}

BasicMesh::~BasicMesh()
{
    clean();
}

void BasicMesh::load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    name = mesh.name;

    const auto& itPos = primitive.attributes.find("POSITION");

    if (itPos != primitive.attributes.end())
    {
        ModuleResources* resources = app->getResources();

        const tinygltf::Accessor& posAcc = model.accessors[itPos->second];

        numVertices = uint32_t(posAcc.count);

        vertices = std::make_unique<Vertex[]>(numVertices);
        uint8_t* vertexData = reinterpret_cast<uint8_t*>(vertices.get());

        loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
        loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
        loadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");

        if(!loadAccessorData(vertexData + offsetof(Vertex, tangent), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "TANGENT"))
        {
            std::vector<Vector4> tangents;
            tangents.resize(numVertices);

            if (loadAccessorData(reinterpret_cast<uint8_t*>(tangents.data()), sizeof(Vector4), sizeof(Vector4), numVertices, model, primitive.attributes, "TANGENT"))
            {
                for (UINT i = 0; i < numVertices; ++i)
                {
                    Vector3& dst = vertices[i].tangent;
                    const Vector4& src = tangents[i];
                    dst.x = src.x;
                    dst.y = src.y;
                    dst.z = src.z*src.w;  
                }
            }
        }
   
        vertexBuffer = resources->createDefaultBuffer(vertices.get(), numVertices * sizeof(Vertex), name.c_str());

        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes  = sizeof(Vertex);
        vertexBufferView.SizeInBytes    = numVertices * sizeof(BasicMesh::Vertex);

        if (primitive.indices >= 0)
        {
            const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];

            _ASSERT_EXPR(indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT || 
                         indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT || 
                         indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE, "Unsupported index format");

            if(indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT ||
               indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
               indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
            {
                static const DXGI_FORMAT formats[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };

                indexElementSize = tinygltf::GetComponentSizeInBytes(indAcc.componentType);
                numIndices = uint32_t(indAcc.count);

                indices = std::make_unique<uint8_t[]>(numIndices*indexElementSize);
                loadAccessorData(indices.get(), indexElementSize, indexElementSize, numIndices, model, primitive.indices);
                indexBuffer = resources->createDefaultBuffer(indices.get(), numIndices * indexElementSize, name.c_str());

                indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
                indexBufferView.Format = formats[indexElementSize >> 1];
                indexBufferView.SizeInBytes = numIndices*indexElementSize;
            }
        }

        materialIndex = primitive.material;
    }
}

void BasicMesh::draw(ID3D12GraphicsCommandList* commandList) const
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    if (indexBuffer)
    {
        commandList->IASetIndexBuffer(&indexBufferView);
        commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
    }
    else
    {
        commandList->DrawInstanced(numVertices, 1, 0, 0);
    }
}

void BasicMesh::clean()
{
    vertexBuffer.Reset();
    indexBuffer.Reset();
}
