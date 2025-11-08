#include "Globals.h"

#include "Scene.h"
#include "Model.h"
#include "QuadTree.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

#include <algorithm>

Scene::Scene()
{
    quadTree = std::make_unique<QuadTree>();
    quadTree->init(3, 64.0f);
}

Scene::~Scene()
{
    _ASSERT_EXPR(models.empty(), "There are live models!!");
}

Model* Scene::loadModel(const char* fileName, const char* basePath)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
    if (loadOk)
    {
        Model* newModel = new Model(this, fileName);
        newModel->load(model, basePath);
        models.push_back(newModel);

        return newModel;
    }

    LOG("Error loading %s: %s", fileName, error.c_str());

    return nullptr;
}

void Scene::onRemoveModel(Model *model)
{
    auto it = std::erase(models, model);
}

void Scene::updateAnimations(float deltaTime)
{
    for (Model* model : models)
    {
        model->updateAnim(deltaTime);
    }
}

void Scene::updateWorldTransforms()
{
    for(Model* model : models)
    {
        model->updateWorldTransforms();
        model->updateQuadTree(quadTree.get());
    }
}

void Scene::frustumCulling(Vector4 planes[6], std::vector<RenderMesh>& renderList)
{

    // First cull quad tree
    std::vector<ContainmentType> containment;
    quadTree->frustumCulling(planes, containment);

    // Then cull models
    for(Model* model : models)
    {
        model->frustumCulling(planes, containment, renderList);
    }
}

void Scene::debugDrawQuadTree(const Vector4 frustumPlanes[6]) const
{
    std::vector<ContainmentType> containment;
    quadTree->frustumCulling(frustumPlanes, containment);
    quadTree->debugDraw(containment);
}

