#pragma once

#include "Module.h"

#include <memory>

class Scene;

class ModuleLevel : public Module
{
public:

    ModuleLevel();
    ~ModuleLevel();

	bool init() override;
	bool cleanUp() override;

    void loadScene(const char* fileName, const char* basePath);
    void cleanScene();

    void updateImGui();

    const Scene* getScene() const {return scene.get(); }
    const Vector3& getLightDir() const {return lightDir;}
    const Vector3& getLightColor() const {return lightColor;}
    const Vector3& getAmbientLightColor() const {return ambientLightColor;}

private:
    std::unique_ptr<Scene> scene;

    Vector3 lightDir = -Vector3::UnitZ;  
    Vector3 lightColor = Vector3::One;
    Vector3 ambientLightColor = Vector3(0.1f, 0.1f, 0.1f);

};