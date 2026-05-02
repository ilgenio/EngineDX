#pragma once

#include "Module.h"

#include <vector>
#include <memory>
#include <span>
#include<set>

class Scene;
class Model;
class Light;
struct Directional;
struct Point;
struct Spot;
class Skybox;
class AnimationClip;
class Decal;

class ModuleScene : public Module
{
    typedef std::vector<std::shared_ptr<AnimationClip>> AnimationClipList;
    typedef std::vector<std::shared_ptr<Model> > ModelList;
    typedef std::vector<std::shared_ptr<Light> > LightList;
    typedef std::vector<std::shared_ptr<Decal> > DecalList;

    AnimationClipList       animations;
    ModelList               models;
    LightList               lights;
    DecalList               decals;
    std::unique_ptr<Scene>  scene;
    std::unique_ptr<Skybox> skybox;

public:

    typedef std::shared_ptr<Model> ModelPtr;
    typedef std::shared_ptr<AnimationClip> ClipPtr;
    typedef std::shared_ptr<Light> LightPtr;
    typedef std::shared_ptr<Decal> DecalPtr;

    typedef std::span<const ModelPtr> ModelSpan;
    typedef std::span<const ClipPtr> ClipSpan;
    typedef std::span<const LightPtr> LightSpan;
    typedef std::span<const DecalPtr> DecalSpan;


    ModuleScene();
    ~ModuleScene();

    bool cleanUp() override;
    void update() override;
    void preRender() override;
    void render() override; 

    Scene* getScene() { return scene.get(); }
    Skybox* getSkybox() { return skybox.get(); }

    const Scene* getScene()  const { return scene.get(); }
    const Skybox* getSkybox() const { return skybox.get(); }

    // Models Management
    UINT      getModelCount() const { return (UINT)models.size(); }
    UINT      addModel(const char* filePath);
    void      removeModel(UINT index) { _ASSERTE(index < models.size()); models.erase(models.begin() + index); }
    ModelPtr  getModel(UINT index) const { _ASSERTE(index < models.size()); return models[index]; }
    ModelSpan getModels() const { return ModelSpan(models.data(), models.size()); }
    void      clearModels() { models.clear(); }


    // Animation Clips Management
    UINT      getClipCount() const { return (UINT)animations.size(); }
    UINT      addClip(const char* filePath, UINT animationIndex = 0);
    void      removeClip(UINT index) { _ASSERTE(index < animations.size()); animations.erase(animations.begin() + index); }
    ClipPtr   getClip(UINT index) const { _ASSERTE(index < animations.size()); return animations[index]; }
    ClipSpan  getClips() const { return ClipSpan(animations.data(), animations.size()); }
    void      clearClips() { animations.clear(); }

    // Lights Management
    UINT      getLightCount() const { return (UINT)lights.size(); }
    UINT      addLight(const Directional& directional);
    UINT      addLight(const Point& point);
    UINT      addLight(const Spot& spot);
    void      removeLight(UINT index) { _ASSERTE(index < lights.size()); lights.erase(lights.begin() + index); }
    LightPtr  getLight(UINT index) const { _ASSERTE(index < lights.size()); return lights[index]; }
    LightSpan getLights() const { return LightSpan(lights.data(), lights.size()); }
    void      clearLights() { lights.clear(); }

    // Decals Management
    UINT      getDecalCount() const { return (UINT)decals.size(); }
    UINT      addDecal(const char* colorPath, const char* normalPath, const char* aoPath, const Matrix& transform);
    void      removeDecal(UINT index) { _ASSERTE(index < decals.size()); decals.erase(decals.begin() + index); }
    DecalPtr  getDecal(UINT index) const { _ASSERTE(index < decals.size()); return decals[index]; }
    DecalSpan getDecals() const { return DecalSpan(decals.data(), decals.size()); }
    void      clearDecals() { decals.clear(); } 

};
