#include "Globals.h"
#include "Mesh.h"

#include "Application.h"
#include "ModuleResources.h"

#include "tiny_gltf.h"

#include "mikktspace.h"
#include "weldmesh.h"

//#define BUILD_TANGENTS_IF_NEEDED
//#define FORCE_WELD

const D3D12_INPUT_ELEMENT_DESC Mesh::inputLayout[numVertexAttribs] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                       {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texCoord0), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                       {"NORMAL",   0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, normal),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                                                       {"TANGENT",  0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, tangent),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

const D3D12_INPUT_LAYOUT_DESC Mesh::inputLayoutDesc = { &inputLayout[0], UINT(std::size(inputLayout)) };

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
    clean();
}

void Mesh::load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    name = mesh.name;

    // TODO: Support not indicexed meshes (weld them ?)
    if (primitive.indices >= 0)
    {
        // NOTE: All our meshes are forced to have positions and normals. They could not have texture coordinates and tangents but will be initialized with them
        const auto& itPos       = primitive.attributes.find("POSITION");
        const auto& itTexCoord  = primitive.attributes.find("TEXCOORD_0");
        const auto& itNormal    = primitive.attributes.find("NORMAL");
        const auto& itTangent   = primitive.attributes.find("TANGENT");

        if (itPos != primitive.attributes.end() && itNormal != primitive.attributes.end() )
        {
            const tinygltf::Accessor& indAcc     = model.accessors[primitive.indices];
            const tinygltf::Accessor& posAcc     = model.accessors[itPos->second];
            const tinygltf::Accessor& normalAcc  = model.accessors[itNormal->second];

            assert(posAcc.count == normalAcc.count);
            assert(posAcc.type == TINYGLTF_TYPE_VEC3);
            assert(normalAcc.type == TINYGLTF_TYPE_VEC3);

            const tinygltf::BufferView& indView         = model.bufferViews[indAcc.bufferView];
            const tinygltf::BufferView& posView         = model.bufferViews[posAcc.bufferView];
            const tinygltf::BufferView& normalView      = model.bufferViews[normalAcc.bufferView];

            const uint8_t* bufferPos      = reinterpret_cast<const uint8_t*>(&(model.buffers[posView.buffer].data[posAcc.byteOffset + posView.byteOffset]));
            const uint8_t* bufferTexCoord = nullptr; 
            const uint8_t* bufferNormal   = reinterpret_cast<const uint8_t*>(&(model.buffers[normalView.buffer].data[normalAcc.byteOffset + normalView.byteOffset]));
            const uint8_t* bufferTangent  = nullptr;

            size_t posStride = posView.byteStride == 0 ? sizeof(Vector3) : posView.byteStride;
            size_t normalStride = normalView.byteStride == 0 ? sizeof(Vector3) : normalView.byteStride;
            size_t texCoordStride = 0; 
            size_t tangentStride = 0;

            if(itTexCoord != primitive.attributes.end())
            {
                const tinygltf::Accessor &texCoordAcc = model.accessors[itTexCoord->second];
                assert(posAcc.count == texCoordAcc.count);
                assert(posAcc.type == TINYGLTF_TYPE_VEC3);
                assert(texCoordAcc.type == TINYGLTF_TYPE_VEC2);

                const tinygltf::BufferView &texCoordView = model.bufferViews[texCoordAcc.bufferView];

                bufferTexCoord = reinterpret_cast<const uint8_t *>(&(model.buffers[texCoordView.buffer].data[texCoordAcc.byteOffset + texCoordView.byteOffset]));
                texCoordStride = texCoordView.byteStride == 0 ? sizeof(Vector2) : texCoordView.byteStride;
            }

            if (itTangent != primitive.attributes.end())
            {
                const tinygltf::Accessor &tangentAcc = model.accessors[itTangent->second];
                assert(tangentAcc.count == normalAcc.count);
                assert(tangentAcc.type == TINYGLTF_TYPE_VEC4);

                const tinygltf::BufferView& tangentView = model.bufferViews[tangentAcc.bufferView];
                bufferTangent = reinterpret_cast<const uint8_t *>(&(model.buffers[tangentView.buffer].data[tangentAcc.byteOffset + tangentView.byteOffset]));
                tangentStride = tangentView.byteStride == 0 ? tangentStride = sizeof(Vector4) : tangentView.byteStride;
            }

            numVertices = uint32_t(posAcc.count);
            numIndices  = uint32_t(indAcc.count);
            vertices    = std::make_unique<Vertex[]>(numVertices);
            indices     = std::make_unique<uint32_t[]>(numIndices);

            // Vertices
            for (uint32_t i = 0; i < numVertices; ++i)
            {
                Vertex& vtx = vertices[i];

                vtx.position = *reinterpret_cast<const Vector3*>(bufferPos);
                vtx.texCoord0 = bufferTexCoord ? *reinterpret_cast<const Vector2*>(bufferTexCoord) : Vector2(0.0f, 0.0f);
                vtx.normal = *reinterpret_cast<const Vector3*>(bufferNormal);
                vtx.tangent = bufferTangent ? *reinterpret_cast<const Vector4*>(bufferTangent) : Vector4(1.0f, 0.0f, 0.0f, 1.0f);

                bufferPos += posStride;
                bufferTexCoord += texCoordStride;
                bufferNormal += normalStride;
                bufferTangent += tangentStride;
            }

            // Indices
            switch(indAcc.componentType)
            {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                {
                    const uint32_t* bufferInd = reinterpret_cast<const uint32_t*>(&(model.buffers[indView.buffer].data[indAcc.byteOffset + indView.byteOffset]));
                    for (uint32_t i = 0; i < numIndices; ++i)
                    {
                        indices[i] = bufferInd[i];
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                {
                    const uint16_t* bufferInd = reinterpret_cast<const uint16_t*>(&(model.buffers[indView.buffer].data[indAcc.byteOffset + indView.byteOffset]));
                    for (uint32_t i = 0; i < numIndices; ++i)
                    {
                        indices[i] = bufferInd[i];
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                {
                    const uint8_t* bufferInd = reinterpret_cast<const uint8_t*>(&(model.buffers[indView.buffer].data[indAcc.byteOffset + indView.byteOffset]));
                    for (uint32_t i = 0; i < numIndices; ++i)
                    {
                        indices[i] = bufferInd[i];
                    }
                    break;
                }
            }

#ifdef BUILD_TANGENTS_IF_NEEDED
            if (itTangent == primitive.attributes.end())
            {
                computeTSpace();
            }
#endif 
#ifdef FORCE_WELD
            weld();
#endif 

            ModuleResources* resources = app->getResources();

            vertexBuffer = resources->createDefaultBuffer(vertices.get(), numVertices * sizeof(Vertex), name.c_str());
            indexBuffer  = resources->createDefaultBuffer(indices.get(), numIndices * sizeof(uint32_t), name.c_str());

            vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
            vertexBufferView.StrideInBytes = sizeof(Vertex);
            vertexBufferView.SizeInBytes = numVertices * sizeof(Mesh::Vertex);

            indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = sizeof(uint32_t);

            // TODO Material Index only in scene
            materialIndex = primitive.material;
        }
    }
}

void Mesh::computeTSpace()
{
    struct UserData
    {
        VertexArray vertices;
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
            Vector4& tangent = reinterpret_cast<const UserData*>(pContext->m_pUserData)->vertices[iFace * 3 + iVert].tangent;
            tangent.x = fvTangent[0];
            tangent.y = fvTangent[1];
            tangent.z = fvTangent[2];
            tangent.w = fvTangent[3];
        }
    };

    UserData userData;

    userData.vertices = std::make_unique<Vertex[]>(numIndices);
    userData.count = numIndices;

    // unweld
    for (uint32_t i = 0; i < numIndices; ++i) userData.vertices[i] = vertices[indices[i]];

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
    vertices = std::make_unique<Vertex[]>(numIndices);
    std::unique_ptr<int[]> remap = std::make_unique<int[]>(numIndices);

    numVertices = uint32_t(WeldMesh(remap.get(), reinterpret_cast<float*>(vertices.get()), reinterpret_cast<const float*>(userData.vertices.get()), numIndices, sizeof(Vertex)/sizeof(float)));
    for (uint32_t i = 0; i < numIndices; ++i)
    {
        indices[i] = uint32_t(remap[i]);
    }
}

void Mesh::weld()
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

void Mesh::clean()
{
    vertexBuffer.Reset();
    indexBuffer.Reset();
}
