#include "Globals.h"

#include "Scene.h"
#include "Mesh.h"
#include "Material.h"
//#include "Skybox.h"

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
}

Scene::~Scene()
{
}

void Scene::loadSkybox(const char* background, const char* diffuse, const char* specular, const char* brdf)
{
    //skybox = std::make_unique<Skybox>();
    //skybox->load(background, diffuse, specular, brdf);
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
    UINT nodeOffset = UINT(nodes.size());
    UINT meshOffset = UINT(meshes.size());
    UINT materialOffset = UINT(materials.size());

    std::vector<int> materialMappings;
    materialMappings.reserve(srcModel.meshes.size());

    for (const auto& srcMesh : srcModel.meshes)
    {
        for (const auto& primitive : srcMesh.primitives)
        {
            Mesh* mesh = new Mesh;
            mesh->load(srcModel, srcMesh, primitive);

            meshes.emplace_back(mesh);

            // TODO: remove material index from mesh
            materialMappings.push_back(primitive.material);
        }
    }

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
            generateNodes(srcModel, nodeIndex, -1, materialMappings, nodeOffset, meshOffset, materialOffset);
        }
    }

    // TODO: Skins

    return true;
}

void Scene::generateNodes(const tinygltf::Model& model, UINT nodeIndex, INT parentIndex, const std::vector<int>& materialMapping, 
                          UINT nodeOffset, UINT meshOffset, UINT materialOffset)
{
    const tinygltf::Node& node = model.nodes[nodeIndex];

    Matrix local = Matrix::Identity;

    if (node.matrix.size() == 16)
    {
        float* ptr = reinterpret_cast<float*>(&local);
        for (UINT i = 0; i < 16; ++i) ptr[i] = float(node.matrix[i]);

        // TODO: Do we need this ? 
        local.Transpose();
    }
    else
    {
        Vector3 translation = Vector3::Zero;
        Vector3 scale = Vector3::One;
        Quaternion rotation = Quaternion::Identity;

        if (node.translation.size() == 3) translation = Vector3(float(node.translation[0]), float(node.translation[1]), float(node.translation[2]));
        if (node.rotation.size() == 4) rotation = Quaternion(float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]), float(node.rotation[3]));
        if (node.scale.size() == 3) scale = Vector3(float(node.scale[0]), float(node.scale[1]), float(node.scale[2]));

        local = Matrix::CreateTranslation(translation) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateScale(scale); 
    }

    Node dst;
    dst.localTransform = local;
    dst.dirtyWorld = true;
    dst.name = node.name;
    dst.parent = parentIndex+nodeOffset;

    if (node.mesh >= 0)
    {
        MeshInstance instance;
        instance.meshIndex     = node.mesh+meshOffset;
        instance.materialIndex = materialMapping[node.mesh]+materialOffset;
        instance.nodeIndex     = nodeIndex+nodeOffset;
        instance.skinIndex     = node.skin;

        instances.push_back(instance);
    }

    nodes.push_back(dst);
    parentIndex = nodeIndex;

    for (int childIndex : node.children)
    {
        generateNodes(model, childIndex, parentIndex, materialMapping, nodeOffset, meshOffset, materialOffset);
    }
}

void Scene::updateWorldTransforms()
{
    for(Node& node : nodes)
    {
        if (node.dirtyWorld)
        {
            INT parentIndex = node.parent;
            if (parentIndex >= 0)
            {
                Node& parent = nodes[parentIndex];
                _ASSERTE(parent.dirtyWorld == false);

                // Parent is clean, so we can use its world transform directly
                node.worldTransform = node.localTransform * parent.worldTransform;
            }
            else
            {
                // No parent, so local is world
                node.worldTransform = node.localTransform;
            }
            node.dirtyWorld = false;
        }
    }
}

void Scene::getRenderList(std::vector<RenderMesh>& renderList) const
{
    renderList.clear();
    renderList.reserve(instances.size());

    for (const auto& instance : instances)
    {
        _ASSERTE(instance.meshIndex < meshes.size());
        _ASSERTE(instance.materialIndex < materials.size());
        _ASSERTE(instance.nodeIndex < nodes.size());

        RenderMesh renderMesh;
        renderMesh.mesh     = meshes[instance.meshIndex];
        renderMesh.material = materials[instance.materialIndex];
        _ASSERTE(nodes[instance.nodeIndex].dirtyWorld == false);

        renderMesh.worldTransform = nodes[instance.nodeIndex].worldTransform;

        renderList.push_back(renderMesh);
    }
}

