#include "Globals.h"

#include "ModuleScene.h"
#include "AnimationClip.h"

#include "Application.h"
#include "Scene.h"
#include "Model.h"
#include "Light.h"           
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
}

void ModuleScene::preRender()
{
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

UINT ModuleScene::addLight(const Directional& directional)
{
    std::shared_ptr<Light> newLight(scene->addLight(directional));
    lights.push_back(newLight);
    return static_cast<UINT>(lights.size() - 1);
}

UINT ModuleScene::addLight(const Point& point)
{
    std::shared_ptr<Light> newLight(scene->addLight(point));
    lights.push_back(newLight);
    return static_cast<UINT>(lights.size() - 1);
}

UINT ModuleScene::addLight(const Spot& spot)
{
    std::shared_ptr<Light> newLight(scene->addLight(spot));
    lights.push_back(newLight);
    return static_cast<UINT>(lights.size() - 1);
}

void ModuleScene::renderDebugDrawModels()
{
    auto drawNode = [](const char* name, const Matrix& worldT, const Matrix& parentT, void* userData)
        {
            dd::line(ddConvert(worldT.Translation()), ddConvert(parentT.Translation()), dd::colors::White, 0, false);

            Vector3 scale;
            Quaternion rotation;
            Vector3 translation;

            worldT.Decompose(scale, rotation, translation);

            Matrix world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(translation);

            dd::axisTriad(ddConvert(world), 0.01f, 0.1f);
        };

    for (UINT modelIdx : debugDrawModels)
    {
        if (modelIdx < models.size())
        {
            std::shared_ptr<const Model> model = models[modelIdx];
            model->enumerateNodes(drawNode);
        }
    }
}

void ModuleScene::renderDebugDrawLights()
{
    for (UINT lightIdx : debugDrawLights)
    {
        if (lightIdx < lights.size())
        {
            std::shared_ptr<const Light> light = lights[lightIdx];
            switch (light->getType())
            {
                case LIGHT_DIRECTIONAL:
                {
                    const float distance = 10.0f;
                    const float size = 0.5f;
                    const Directional& dirLight = light->getDirectional();
                    Vector3 color(dirLight.Lc.x * dirLight.Lc.w, dirLight.Lc.y * dirLight.Lc.w, dirLight.Lc.z * dirLight.Lc.w);
                    dd::arrow(ddConvert(-dirLight.Ld * (distance + size)), ddConvert(-dirLight.Ld * distance), ddConvert(color), 0.1f);
                    break;
                }
                case LIGHT_POINT:
                {
                    const Point& pointLight = light->getPoint();
                    Vector3 color(pointLight.Lc.x * pointLight.Lc.w, pointLight.Lc.y * pointLight.Lc.w, pointLight.Lc.z * pointLight.Lc.w);
                    dd::sphere(ddConvert(pointLight.Lp), ddConvert(color), sqrtf(pointLight.sqRadius));
                    break;
                }
                case LIGHT_SPOT:
                {
                    const Spot& spotLight = light->getSpot();
                    Vector3 color(spotLight.Lc.x * spotLight.Lc.w, spotLight.Lc.y * spotLight.Lc.w, spotLight.Lc.z * spotLight.Lc.w);
                    dd::arrow(ddConvert(spotLight.Lp), ddConvert(spotLight.Lp + spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(color), 0.1f);
                    dd::cone(ddConvert(spotLight.Lp), ddConvert(spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(color), sqrtf(spotLight.sqRadius) * tanf(acosf(spotLight.outer)), 0.0f);
                    break;
                }
            }
        }
    }
}

