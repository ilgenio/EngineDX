#include "Globals.h"

#include "DemoSkinning.h"

#include "Application.h"
#include "ModuleCamera.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

DemoSkinning::DemoSkinning()
{

}

DemoSkinning::~DemoSkinning()
{

}

bool DemoSkinning::init() 
{
    bool ok = Demo::init();

    if(ok)
    {
        scene = std::make_unique<Scene>();
        model.reset(scene->loadModel("Assets/Models/KyleRobot/KyleRobot.gltf", "Assets/Models/KyleRobot/"));
        //model.reset(scene->loadModel("Assets/Models/BistroExterior/BistroExterior.gltf", "Assets/Models/BistroExterior/"));    

        bool ok = model.get();

        if (ok)
        {
            //animation = std::make_shared<AnimationClip>();
            //animation->load("Assets/Models/busterDrone/busterDrone.gltf", 0);

            //model->PlayAnim(animation);
        }
        
        skybox = std::make_unique<Skybox>();
        
        ok = ok && skybox->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

        _ASSERT_EXPR(ok, L"Error loading scene");

    }

    if(ok)
    {
        ModuleCamera* camera = app->getCamera();

        camera->setPolar(XMConvertToRadians(1.30f));
        camera->setAzimuthal(XMConvertToRadians(-11.61f));
        camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));
    }

    return ok;
}
