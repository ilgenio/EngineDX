#include "Globals.h"

#include "Scene.h"
#include "Model.h"

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

void Scene::updateWorldTransforms()
{
    for(Model* model : models)
    {
        model->updateWorldTransforms();
    }
}

void Scene::getRenderList(std::vector<RenderMesh>& renderList) const
{
    UINT totalInstances = 0;
    for(Model* model : models)
    {
        totalInstances += model->getNumInstances();
    }

    renderList.clear();
    renderList.reserve(totalInstances);

    for(Model* model : models)
    {
        model->getRenderList(renderList);
    }
}

