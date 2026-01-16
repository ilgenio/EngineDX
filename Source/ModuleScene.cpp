#include "Globals.h"

#include "ModuleScene.h"
#include "AnimationClip.h"

#include "Application.h"
#include "Scene.h"
#include "Model.h"
#include "Skybox.h"

ModuleScene::ModuleScene()
{
    scene = std::make_unique<Scene>();
    skybox = std::make_unique<Skybox>();
}

ModuleScene::~ModuleScene()
{
}

bool ModuleScene::cleanUp()
{
    models.clear();
    animations.clear();

    skybox.reset();
    scene.reset();
    models.clear();
    animations.clear();

    return true;
}

void ModuleScene::update()
{
    // Update scene
    scene->updateAnimations(float(app->getElapsedMilis()) * 0.001f);
    scene->updateWorldTransforms();
}

UINT ModuleScene::addModel(const char* filePath, const char* basePath)
{
    std::shared_ptr<Model> newModel(scene->loadModel(filePath, basePath));

    models.push_back(newModel);
    return static_cast<UINT>(models.size() - 1);
}

UINT ModuleScene::addClip(const char* filePath, UINT animationIndex)
{
    std::shared_ptr<AnimationClip> newClip = std::make_shared<AnimationClip>();
    newClip->load(filePath, animationIndex);

    animations.push_back(newClip);
    return static_cast<UINT>(animations.size() - 1);
}