#include "Globals.h"

#include "Scene.h"
#include "Mesh.h"
#include "Material.h"
//#include "Skybox.h"

#include "tiny_gltf.h"


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

void Scene::load(const tinygltf::Model &srcModel, const char *basePath)
{
    meshes.reserve(srcModel.meshes.size());

    for (const auto& srcMesh : srcModel.meshes)
    {
        for (const auto& primitive : srcMesh.primitives)
        {
            Mesh* mesh = new Mesh;
            mesh->load(srcModel, srcMesh, primitive);

            meshes.emplace_back(mesh);
        }
    }

    materials.reserve(srcModel.materials.size());

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
            generateInstancesRec(srcModel, nodeIndex, Matrix::Identity);
        }
    }
}

void Scene::generateInstancesRec(const tinygltf::Model& srcModel, int nodeIndex, const Matrix& transform)
{
    const tinygltf::Node& node = srcModel.nodes[nodeIndex];
    
    Matrix local = Matrix::Identity;

    if (node.matrix.size() == 16)
    {
        int k = 0;
        for (uint32_t i = 0; i < 4; ++i)
            for (uint32_t j = 0; j < 4; ++j)
                local.m[j][i] = float(node.matrix[k++]);
    }
    else
    {
        Vector3 translation = Vector3::Zero, scale = Vector3::One;
        Quaternion rotation = Quaternion::Identity;

        if (node.translation.size() == 3)
        {
            translation.x = float(node.translation[0]);
            translation.y = float(node.translation[1]);
            translation.z = float(node.translation[2]);
        }
        if (node.rotation.size() == 4)
        {
            rotation.x = float(node.rotation[0]);
            rotation.y = float(node.rotation[1]);
            rotation.z = float(node.rotation[2]);
            rotation.w = float(node.rotation[3]);
        }
        if (node.scale.size() == 3)
        {
            scale.x = float(node.scale[0]);
            scale.y = float(node.scale[1]);
            scale.z = float(node.scale[2]);
        }
        
        Matrix matrix = matrix.CreateFromQuaternion(rotation);
        matrix.Translation(translation);
        matrix.Right(scale.x * matrix.Right());
        matrix.Up(scale.y * matrix.Up());
        matrix.Forward(scale.z * matrix.Forward());
    }

    Matrix newTransform = local * transform;

    if(node.mesh >= 0)
    {
        MeshInstance instance = { (uint32_t)node.mesh, instance.transformation = newTransform };

        instances.push_back(instance);
    }

    for(int childIndex : node.children)
    {
        generateInstancesRec(srcModel, childIndex, newTransform);
    }
}

