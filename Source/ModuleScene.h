#pragma once

#include "Module.h"

#include <vector>
#include <memory>
#include <span>

class Scene;
class Model;
class Skybox;
class AnimationClip;

class ModuleScene : public Module
{
    typedef std::vector<std::shared_ptr<AnimationClip>> AnimationClipList;
    typedef std::vector<std::shared_ptr<Model> > ModelList;

    AnimationClipList       animations;
    ModelList               models;

    std::unique_ptr<Scene>  scene;
    std::unique_ptr<Skybox> skybox;

public:

    typedef std::shared_ptr<Model> ModelPtr;
    typedef std::shared_ptr<AnimationClip> ClipPtr;
    typedef std::span<const ModelPtr> ModelSpan;
    typedef std::span<const ClipPtr> ClipSpan;


    ModuleScene();
    ~ModuleScene();

    bool cleanUp() override;
    void update();

    Scene* getScene() { return scene.get(); }
    Skybox* getSkybox() { return skybox.get(); }

    const Scene* getScene()  const { return scene.get(); }
    const Skybox* getSkybox() const { return skybox.get(); }

    // Models Management
    UINT      getModelCount() const { return (UINT)models.size(); }
    UINT      addModel(const char* filePath, const char* basePath);
    void      removeModel(UINT index) { _ASSERTE(index < models.size()); models.erase(models.begin() + index); }
    ModelPtr  getModel(UINT index) const { _ASSERTE(index < models.size()); return models[index]; }
    ModelSpan getModels() const { return ModelSpan(models.data(), models.size()); }
    void      clearModels() { models.clear(); }


    // Animation Clips Management
    UINT      getClipCount() const { return (UINT)animations.size(); }
    UINT      addClip(const char* filePath, UINT animationIndex = 0);
    void      removeClip(UINT index) { _ASSERTE(index < animations.size()); animations.erase(animations.begin() + index); }
    ClipPtr   getClip(UINT index) const { _ASSERTE(index < animations.size()); return animations[index]; }
    ClipSpan  getClips() const { return animations; }
    void      clearClips() { animations.clear(); }

private:

};
