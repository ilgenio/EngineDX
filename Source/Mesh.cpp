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

        std::unique_ptr<SkinBoneData[]> bones = std::make_unique<SkinBoneData[]>(numVertices);
        uint8_t* boneDataPtr = reinterpret_cast<uint8_t*>(bones.get());

        bool hasJoints = false;

        const auto& it = primitive.attributes.find("JOINTS_0");
        if (it != primitive.attributes.end())
        {
            size_t jointSize = getAccessorElementSize(model, it->second); 

            if (jointSize == sizeof(UINT) * 4)
            {
                hasJoints = loadAccessorData(boneDataPtr + offsetof(SkinBoneData, indices), sizeof(UINT) * 4, sizeof(SkinBoneData), numVertices, model, it->second);
            }
            else if (jointSize == sizeof(SHORT) * 4)
            {
                struct ShortIndices { SHORT indices[4]; };
                std::unique_ptr<ShortIndices[]> shortData;
                UINT numJointIndices = 0;
                hasJoints = loadAccessorTyped(shortData, numJointIndices, model, it->second);
                _ASSERT(numJointIndices == numVertices);
                for (UINT i=0; i< numJointIndices; ++i)
                {
                    for (UINT j = 0; j < 4; ++j)
                    {
                        bones[i].indices[j] = UINT(shortData[i].indices[j]);
                    }
                }
            }
        }

        bool hasWeights = loadAccessorData(boneDataPtr + offsetof(SkinBoneData, weights), sizeof(Vector4), sizeof(SkinBoneData), numVertices, model, primitive.attributes, "WEIGHTS_0");

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
            computeTSpace(vertices, indices, bones);
        }

        if(numIndices > 0)
        {
            staticBuffer->allocIndexBuffer(numIndices, indexElementSize, indices.get(), indexBufferView);
        }

        staticBuffer->allocVertexBuffer(numVertices, sizeof(Vertex), vertices.get(), vertexBufferView);

        if (hasJoints && hasWeights)
        {
            staticBuffer->allocBuffer(numVertices * sizeof(SkinBoneData), &bones[0], boneData);
        }
    }
}

void Mesh::computeTSpace(std::unique_ptr<Vertex[]>& vertices, std::unique_ptr<uint8_t[]>& indices, std::unique_ptr<SkinBoneData[]>& bones)
    
{
    struct UserData
    {
        std::unique_ptr<Vertex[]> vertices;
        std::unique_ptr<SkinBoneData[]> bones;
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
    userData.bones = std::make_unique<SkinBoneData[]>(numIndices);
    userData.count = numIndices;

    auto unweld = [](auto* indices, UINT count, Vertex* outVertices, SkinBoneData* outBones, Vertex* inVertices, SkinBoneData* inBones)
        {
            for (UINT i = 0; i < count; ++i)
            {
                outVertices[i] = inVertices[indices[i]];
                outBones[i] = inBones[indices[i]];
                indices[i] = i;
            }
        };

    switch (indexElementSize)
    {
    case 1: unweld(indices.get(), numIndices, userData.vertices.get(), userData.bones.get(), vertices.get(), bones.get()); break;
    case 2: unweld(reinterpret_cast<uint16_t*>(indices.get()), numIndices, userData.vertices.get(), userData.bones.get(), vertices.get(), bones.get()); break;
    case 4: unweld(reinterpret_cast<uint32_t*>(indices.get()), numIndices, userData.vertices.get(), userData.bones.get(), vertices.get(), bones.get()); break;
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

    int* remap = reinterpret_cast<int*>(&indices[0]);

    numVertices = uint32_t(WeldMesh(remap, reinterpret_cast<float*>(weldVertices.get()), reinterpret_cast<const float*>(userData.vertices.get()), numIndices, sizeof(Vertex) / sizeof(float)));

    vertices = std::make_unique<Vertex[]>(numVertices);
    memcpy(vertices.get(), weldVertices.get(), sizeof(Vertex)* numVertices);

    bones = std::make_unique<SkinBoneData[]>(numVertices);

    for (uint32_t i = 0; i < numIndices; ++i)
    {
        bones[remap[i]] = userData.bones[i];
    }
}

