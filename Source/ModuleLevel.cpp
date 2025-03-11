#include "Globals.h"
#include "ModuleLevel.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include "Scene.h"
#include "Mesh.h"
#include "Material.h"

//#include <imgui.h>


#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"

ModuleLevel::ModuleLevel()
{

}

ModuleLevel::~ModuleLevel()
{
}

bool ModuleLevel::init() 
{
    Material::createSharedData();

    //const char* assetFileName = "Assets/Models/IridescenceSuzanne/IridescenceSuzanne.gltf"; 
    //const char* basePath = "Assets/Models/IridescenceSuzanne/";
    //const char* assetFileName = "Assets/Models/SciFiHelmet/SciFiHelmet.gltf";
    //const char* basePath = "Assets/Models/SciFiHelmet/";
    const char* assetFileName = "Assets/Models/MetalRoughSpheres/MetalRoughSpheres.gltf";
    const char* basePath = "Assets/Models/MetalRoughSpheres/";
    //const char* assetFileName = "Assets/Models/IridescenceMetallicSpheres/IridescenceMetallicSpheres.gltf";
    //const char* basePath = "Assets/Models/IridescenceMetallicSpheres/";
    //const char* assetFileName = "Assets/Models/IridescenceDielectricSpheres/IridescenceDielectricSpheres.gltf";
    //const char* basePath = "Assets/Models/IridescenceDielectricSpheres/";

    loadScene(assetFileName, basePath);


    return true;
}

bool ModuleLevel::cleanUp() 
{
    scene.reset();

    Material::destroySharedData();

    return true;
}

void ModuleLevel::loadScene(const char* assetFileName, const char* basePath)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, assetFileName);

    if (loadOk)
    {
        scene = std::make_unique<Scene>();
        scene->load(model, basePath);
    }
    else
    {
        LOG("Error loading %s: %s", assetFileName, error.c_str());
    }

#if 0
    if (loadOk) scene->loadSkybox("Assets/Textures/SkyKTX/footprint_court/skybox.hdr", 
                                  "Assets/Textures/SkyKTX/footprint_court/diffuse.ktx2",
                                  "Assets/Textures/SkyKTX/footprint_court/specular.ktx2",
                                  "Assets/Textures/SkyKTX/footprint_court/outputLUT.png");
#endif 
}

void ModuleLevel::cleanScene()
{
    scene.reset();
}

#if 0
void ModuleLevel::updateImGui()
{
    ImGui::Begin("Level");

    float polar   = atan2f(sqrtf(lightDir.x*lightDir.x+lightDir.z*lightDir.z), lightDir.y);
    float azimuth = atan2f(lightDir.z, lightDir.x);

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Light Direction %g %g %g", lightDir.x, lightDir.y, lightDir.z);

    float epsilon = 0.00001f; // undefined at pole

    if (ImGui::DragFloat("Polar [0 PI]", &polar, 0.01f, epsilon, glm::pi<float>()-epsilon, "%.5f") ||
        ImGui::DragFloat("Azimuth [-PI PI]", &azimuth, 0.01f, -glm::pi<float>(), glm::pi<float>(), "%.5f"))
    {
        lightDir.x = sin(polar) * cos(azimuth);
        lightDir.y = cos(polar);
        lightDir.z = sin(polar) * sin(azimuth);

        lightDir = glm::normalize(lightDir);
    }

    ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&lightColor));
    ImGui::ColorEdit3("Ambient Light Color", reinterpret_cast<float*>(&ambientLightColor));
    ImGui::End();
}
#endif
