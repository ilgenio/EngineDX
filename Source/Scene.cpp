#include "Globals.h"

#include "Scene.h"
#include "Mesh.h"
#include "Material.h"
#include "Skybox.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

Scene::Scene()
{
    skybox = std::make_unique<Skybox>();
}

Scene::~Scene()
{
    for (Mesh* mesh : meshes) delete mesh;
    for (Material* material : materials) delete material;
    for (Node* node : nodes) delete node;
    for (MeshInstance* instance : instances) delete instance;
}

bool Scene::loadSkyboxHDR(const char* hdrFileName)
{
    return skybox->loadHDR(hdrFileName);
}

bool Scene::load(const char* fileName, const char* basePath)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
    if (loadOk)
    {
        return load(model, basePath);
    }

    LOG("Error loading %s: %s", fileName, error.c_str());

    return false;
}

bool Scene::load(const tinygltf::Model& srcModel, const char* basePath)
{
    UINT meshOffset = UINT(meshes.size());
    UINT materialOffset = UINT(materials.size());

    std::vector<int> materialMappings;
    materialMappings.reserve(srcModel.meshes.size());

    std::vector<std::pair<UINT, UINT>> meshMappings;
    meshMappings.reserve(srcModel.meshes.size());

    for (const auto& srcMesh : srcModel.meshes)
    {
        meshMappings.push_back({ int(materialMappings.size()), int(srcMesh.primitives.size()) });

        for (const auto& primitive : srcMesh.primitives)
        {
            Mesh* mesh = new Mesh;
            mesh->load(srcModel, srcMesh, primitive);

            meshes.push_back(mesh);

            // TODO: remove material index from mesh
            materialMappings.push_back(primitive.material);
        }
    }

    _ASSERTE(meshes.size() == materialMappings.size());


    MaterialList tmpMaterials;
    tmpMaterials.reserve(srcModel.materials.size());

    for(const auto& srcMaterial : srcModel.materials)
    {
        Material* material = new Material;

        material->load(srcModel, srcMaterial, basePath);

        materials.emplace_back(material);
    }


    for(const tinygltf::Scene& scene : srcModel.scenes)
    {
        for (int nodeIndex : srcModel.scenes[0].nodes)
        {
            generateNodes(srcModel, nodeIndex, -1, meshMappings, materialMappings, meshOffset, materialOffset);
        }
    }

    // TODO: Skins

    return true;
}

UINT Scene::generateNodes(const tinygltf::Model& model, UINT nodeIndex, INT parentIndex, 
                          const std::vector<std::pair<UINT, UINT>>& meshMapping, 
                          const std::vector<int>& materialMapping, UINT meshOffset, 
                          UINT materialOffset)
{
    const tinygltf::Node& node = model.nodes[nodeIndex];

    Matrix local = Matrix::Identity;

    if (node.matrix.size() == 16)
    {
        float* ptr = reinterpret_cast<float*>(&local);
        for (UINT i = 0; i < 16; ++i) ptr[i] = float(node.matrix[i]);

        // Column major + post multiply  So don't need to be transposed because we are row major + pre multiply
    }
    else
    {
        Vector3 translation = Vector3::Zero;
        Vector3 scale = Vector3::One;
        Quaternion rotation = Quaternion::Identity;

        if (node.translation.size() == 3) translation = Vector3(float(node.translation[0]), float(node.translation[1]), float(node.translation[2]));
        if (node.rotation.size() == 4) rotation = Quaternion(float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]), float(node.rotation[3]));
        if (node.scale.size() == 3) scale = Vector3(float(node.scale[0]), float(node.scale[1]), float(node.scale[2]));

        local = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(translation);
    }

    // Note: nodes are guaranteed to be stored in preorder

    Node* dst = new Node;
    dst->localTransform = local;
    dst->dirtyWorld = true;
    dst->name = node.name;
    dst->parent = parentIndex;
    dst->numChilds = UINT(node.children.size());

    UINT realIndex = UINT(nodes.size());

    if (node.mesh >= 0)
    {
        // as we have one mesh per primitive one gltf mesh generates more than one of our meshes
        for(UINT i = 0; i < meshMapping[node.mesh].second; ++i)
        {
            // TODO: assign pointers instead of indices
            MeshInstance* instance = new MeshInstance;
            instance->meshIndex     = UINT(meshMapping[node.mesh].first + i + meshOffset);
            instance->materialIndex = UINT(materialMapping[meshMapping[node.mesh].first + i] + materialOffset);
            instance->nodeIndex     = realIndex;
            instance->skinIndex     = node.skin;

            _ASSERTE(instance->meshIndex < meshes.size());
            _ASSERTE(instance->materialIndex < materials.size());

            instances.push_back(instance);
        }
    }

    nodes.push_back(dst);

    for (int childIndex : node.children)
    {
        dst->numChilds += generateNodes(model, childIndex, realIndex, meshMapping, materialMapping, meshOffset, materialOffset);
    }

    return dst->numChilds;
}

void Scene::updateWorldTransforms()
{
    for(Node* node : nodes)
    {
        if (node->dirtyWorld)
        {
            INT parentIndex = node->parent;
            if (parentIndex >= 0)
            {
                Node* parent = nodes[parentIndex];
                _ASSERTE(parent->dirtyWorld == false);

                // Parent is clean, so we can use its world transform directly
                node->worldTransform = node->localTransform * parent->worldTransform;
            }
            else
            {
                // No parent, so local is world
                node->worldTransform = node->localTransform;
            }
            node->dirtyWorld = false;
        }
    }
}

void Scene::getRenderList(std::vector<RenderMesh>& renderList) const
{
    renderList.clear();
    renderList.reserve(instances.size());

    for (const MeshInstance* instance : instances)
    {
        _ASSERTE(instance->meshIndex < meshes.size());
        _ASSERTE(instance->materialIndex < materials.size());
        _ASSERTE(instance->nodeIndex < nodes.size());

        RenderMesh renderMesh;
        renderMesh.mesh     = meshes[instance->meshIndex];
        renderMesh.material = materials[instance->materialIndex];

        _ASSERTE(nodes[instance->nodeIndex]->dirtyWorld == false);

        renderMesh.transform = nodes[instance->nodeIndex]->worldTransform;

        renderList.push_back(renderMesh);
    }
}

