#include "Globals.h"
#include "Mesh.h"

#include "Application.h"
#include "ModuleStaticBuffer.h"

#include "gltf_utils.h"

#include "mikktspace.h"
#include "weldmesh.h"

const D3D12_INPUT_ELEMENT_DESC Mesh::inputLayout[numVertexAttribs] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                       {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texCoord0), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                       {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                       {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

const D3D12_INPUT_LAYOUT_DESC Mesh::inputLayoutDesc = { &inputLayout[0], UINT(std::size(inputLayout)) };


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    name = mesh.name;

    const auto& itPos = primitive.attributes.find("POSITION");

    if (itPos != primitive.attributes.end())
    {
        ModuleStaticBuffer* staticBuffer = app->getStaticBuffer();

        const tinygltf::Accessor& posAcc = model.accessors[itPos->second];

        numVertices = uint32_t(posAcc.count);

        std::unique_ptr<uint8_t[]> indices;
        std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(numVertices);
        uint8_t* vertexData = reinterpret_cast<uint8_t*>(vertices.get());

        loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
        loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
        loadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");

        bool hasTangents = loadAccessorData(vertexData + offsetof(Vertex, tangent), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "TANGENT");
        if (!hasTangents)
        {
            std::vector<Vector4> tangents;
            tangents.resize(numVertices);

            hasTangents = loadAccessorData(reinterpret_cast<uint8_t*>(tangents.data()), sizeof(Vector4), sizeof(Vector4), numVertices, model, primitive.attributes, "TANGENT");
            if (hasTangents)
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

        if (numVertices > 0)
        {
            BoundingBox::CreateFromPoints(bbox, numVertices, &vertices[0].position, sizeof(Vertex));
        }

        // Skinning attributes

        struct BoneIndices
        {
            UINT indices[4];
        };

        UINT numJoints, numWeights;
        std::unique_ptr<BoneIndices []> boneIndexArray;
        std::unique_ptr<Vector4 []> boneWeightArray;

        loadAccessorTyped(boneIndexArray, numJoints, model, primitive.attributes, "JOINTS_0");
        loadAccessorTyped(boneWeightArray, numWeights, model, primitive.attributes, "WEIGHTS_0");

        _ASSERTE(numJoints == 0 || numJoints == numVertices);
        _ASSERTE(numWeights == 0 || numWeights == numVertices);

        if (primitive.indices >= 0)
        {
            const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];

            _ASSERT_EXPR(indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT ||
                indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
                indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE, "Unsupported index format");

            if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT ||
                indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
                indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
            {
                indexElementSize = tinygltf::GetComponentSizeInBytes(indAcc.componentType);
                numIndices = uint32_t(indAcc.count);

                indices = std::make_unique<uint8_t[]>(numIndices * indexElementSize);
                loadAccessorData(indices.get(), indexElementSize, indexElementSize, numIndices, model, primitive.indices);
            }
        }

        if (!hasTangents)
        {
            computeTSpace(vertices, indices);
        }

        if(numIndices > 0)
        {
            staticBuffer->allocIndexBuffer(numIndices, indexElementSize, indices.get(), indexBufferView);
        }

        staticBuffer->allocVertexBuffer(numVertices, sizeof(Vertex), vertices.get(), vertexBufferView);

        if (numJoints == numVertices && numWeights == numVertices)
        {
            staticBuffer->allocBuffer(numJoints * sizeof(BoneIndices), &boneIndexArray[0], boneIndices);
            staticBuffer->allocBuffer(numWeights * sizeof(Vector4), &boneWeightArray[0], boneWeights);
        }
    }
}

void Mesh::draw(ID3D12GraphicsCommandList* commandList) const
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    if (numIndices > 0)
    {
        commandList->IASetIndexBuffer(&indexBufferView);
        commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
    }
    else
    {
        commandList->DrawInstanced(numVertices, 1, 0, 0);
    }
}

void Mesh::computeTSpace(std::unique_ptr<Vertex[]>& vertices, std::unique_ptr<uint8_t[]>& indices)
{
    struct UserData
    {
        std::unique_ptr<Vertex[]> vertices;
        uint32_t count;
    };

    struct TIFace
    {
        static int getNumFaces(const SMikkTSpaceContext* pContext)
        {
            return reinterpret_cast<UserData*>(pContext->m_pUserData)->count / 3;
        }

        static int getNumVerticesOfFace(const SMikkTSpaceContext*, const int )
        {
            return 3;
        }

        static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            const Vector3& v = reinterpret_cast<const UserData*>(pContext->m_pUserData)->vertices[iFace * 3 + iVert].position;
            fvPosOut[0] = v.x;
            fvPosOut[1] = v.y;
            fvPosOut[2] = v.z;
        }

        static void getNormal(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            const Vector3& v = reinterpret_cast<const UserData*>(pContext->m_pUserData)->vertices[iFace * 3 + iVert].normal;
            fvPosOut[0] = v.x;
            fvPosOut[1] = v.y;
            fvPosOut[2] = v.z;
        }

        static void getTexCooord(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
        {
            const Vector2& v = reinterpret_cast<const UserData*>(pContext->m_pUserData)->vertices[iFace * 3 + iVert].texCoord0;
            fvPosOut[0] = v.x;
            fvPosOut[1] = v.y;
        }
        static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
        {
            Vector3& tangent = reinterpret_cast<const UserData*>(pContext->m_pUserData)->vertices[iFace * 3 + iVert].tangent;
            tangent.x = fvTangent[0];
            tangent.y = fvTangent[1];
            tangent.z = fSign*fvTangent[2];
        }
    };

    UserData userData;

    userData.vertices = std::make_unique<Vertex[]>(numIndices);
    userData.count = numIndices;

    auto unweld = [](auto* indices, UINT count, Vertex* outVertices, Vertex* inVertices)
        {
            for (UINT i = 0; i < count; ++i)
            {
                outVertices[i] = inVertices[indices[i]];
                indices[i] = i;
            }
        };

    switch (indexElementSize)
    {
    case 1: unweld(indices.get(), numIndices, userData.vertices.get(), vertices.get()); break;
    case 2: unweld(reinterpret_cast<uint16_t*>(indices.get()), numIndices, userData.vertices.get(), vertices.get()); break;
    case 4: unweld(reinterpret_cast<uint32_t*>(indices.get()), numIndices, userData.vertices.get(), vertices.get()); break;
    }

    SMikkTSpaceInterface iface;
    iface.m_getNumFaces = &TIFace::getNumFaces;
    iface.m_getNumVerticesOfFace = &TIFace::getNumVerticesOfFace;
    iface.m_getPosition = &TIFace::getPosition;
    iface.m_getNormal = &TIFace::getNormal;
    iface.m_getTexCoord = &TIFace::getTexCooord;
    iface.m_setTSpace = nullptr;
    iface.m_setTSpaceBasic= &TIFace::setTSpaceBasic;

    SMikkTSpaceContext context;
    context.m_pInterface = &iface;
    context.m_pUserData = reinterpret_cast<void*>(&userData);

    genTangSpaceDefault(&context);
    
    // Weld 
    std::unique_ptr<Vertex[]> weldVertices = std::make_unique<Vertex[]>(numIndices);

    indexElementSize = 4;
    indices = std::make_unique<uint8_t[]>(numIndices* indexElementSize);

    int* data = reinterpret_cast<int*>(&indices[0]);

    numVertices = uint32_t(WeldMesh(data, reinterpret_cast<float*>(weldVertices.get()), reinterpret_cast<const float*>(userData.vertices.get()), numIndices, sizeof(Vertex)/sizeof(float)));

    vertices = std::make_unique<Vertex[]>(numVertices);
    memcpy(vertices.get(), weldVertices.get(), sizeof(Vertex)* numVertices);

    weld(vertices, indices);
}

void Mesh::weld(std::unique_ptr<Vertex[]>& vertices, std::unique_ptr<uint8_t[]> &indices)
{
    std::unique_ptr<Vertex[]> new_vertices = std::make_unique<Vertex[]>(numVertices);
    std::unique_ptr<int[]> remap = std::make_unique<int[]>(numVertices);

    numVertices = uint32_t(WeldMesh(remap.get(), reinterpret_cast<float*>(new_vertices.get()), reinterpret_cast<const float*>(vertices.get()), numVertices, sizeof(Vertex) / sizeof(float)));
    for (uint32_t i = 0; i < numIndices; ++i)
    {
        indices[i] = uint32_t(remap[indices[i]]);
    }

    std::swap(vertices, new_vertices);
}
