#include "Globals.h"

#include "ModuleSceneEditor.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleCamera.h"

#include "MathUtils.h"

#include "Light.h"
#include "Model.h"
#include "Scene.h"
#include "Skybox.h"
#include "Decal.h"

namespace
{
    bool imGuiDirection(Vector3& dir)
    {
        float azimuth;
        float elevation;
        euclideanToSpherical(dir, azimuth, elevation);

        while (azimuth < 0.0f) azimuth += M_TWO_PI;
        while (elevation < 0.0f) elevation += M_TWO_PI;

        ImGui::Text("Direction (%g, %g, %g)", dir.x, dir.y, dir.z);
        bool change = ImGui::SliderAngle("Dir azimuth", &azimuth, 0.0f, 360.0f);
        change = ImGui::SliderAngle("Dir elevation", &elevation, 1.0f, 179.0f) || change;

        if (change)
        {
            sphericalToEuclidean(azimuth, elevation, dir);
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Normalize"))
        {
            dir.Normalize();
            change = true;
        }

        return change;  
    }
}


ModuleSceneEditor::ModuleSceneEditor()
{

}

ModuleSceneEditor::~ModuleSceneEditor()
{
}

void ModuleSceneEditor::render()
{
    debugDrawCommands();
    imGuiDrawObjects();
    imGuiDrawProperties();
}

void ModuleSceneEditor::debugDrawCommands()
{
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);

    float aspect = app->getRender()->getRenderTargetAspect();

    if (trackFrustum && aspect > 0.0f)
    {
        app->getCamera()->getFrustumPlanes(frustumPlanes, aspect, true);
        trackedFrustum = app->getCamera()->getFrustum(aspect);
    }

    if (showQuadTree)
    {
        app->getScene()->getScene()->debugDrawQuadTree(frustumPlanes, quadTreeLevel);

        Vector3 points[8];
        trackedFrustum.GetCorners(points);
        dd::box(ddConvert(points), dd::colors::White);
    }

    switch (selectionType)
    {
    case SELECTION_MODEL:
        renderDebugDrawModel();
        break;
    case SELECTION_LIGHT:
        renderDebugDrawLight();
        break;
    case SELECTION_DECAL:
        renderDebugDrawDecal();
        break;
    }
}


void ModuleSceneEditor::imGuiDrawObjects()
{
    ModuleScene* scene = app->getScene();
    ModuleRender* render = app->getRender();

    ImGui::Begin("Objects");

    if (ImGui::CollapsingHeader("Models", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const auto& models = scene->getModels();        
        UINT index = 0;
        for (const auto& model : models)
        {
            bool selected = (index == selectedIndex) && selectionType == SELECTION_MODEL;
            if (selected)
            {
                model->setRootTransform(render->getGuizmoTransform());
            }

            if(ImGui::Selectable(model->getName().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (selected)
                {
                    selectionType = SELECTION_NONE;
                    render->setShowGuizmo(false);
                }
                else
                {
                    selectedIndex = index;
                    selectionType = SELECTION_MODEL;

                    render->setShowGuizmo(true);
                    render->setGuizmoTransform(model->getRootTransform());
                }
            }

            ++index;
        }
    }

    if(ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        UINT dirIndex = 0;
        UINT pointIndex = 0;
        UINT spotIndex = 0;
        for (UINT index = 0; index < scene->getLightCount(); )
        {
            const auto light = scene->getLight(index);

            std::string lightName;
            switch (light->getType())
            {
                case LIGHT_DIRECTIONAL:
                    lightName = "Directional " + std::to_string(dirIndex++);
                    break;
                case LIGHT_POINT:
                    lightName = "Point " + std::to_string(pointIndex++);
                    break;
                case LIGHT_SPOT:
                    lightName = "Spot " + std::to_string(spotIndex++);
                    break;
            }
            
            bool selected = (index == selectedIndex) && selectionType == SELECTION_LIGHT;
            if (selected)
            {
                if (light->getType() == LIGHT_POINT)
                {
                    Point point = light->getPoint();
                    point.Lp = render->getGuizmoTransform().Translation();
                    light->setPoint(point);
                }
                else if (light->getType() == LIGHT_SPOT)
                {
                    Spot spot = light->getSpot();
                    const Matrix& transform = render->getGuizmoTransform();
                    spot.Lp = transform.Translation();
                    spot.Ld = transform.Backward();
                    light->setSpot(spot);
                }
            }

            ImGui::PushID(index);
            if (ImGui::Selectable(lightName.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (selected)
                {
                    selectionType = SELECTION_NONE;
                    render->setShowGuizmo(false);
                }
                else
                {
                    selectedIndex = index;
                    selectionType = SELECTION_LIGHT;

                    if (light->getType() == LIGHT_POINT)
                    {
                        Matrix transform = Matrix::Identity;
                        transform.Translation(light->getPoint().Lp);

                        render->setShowGuizmo(true);
                        render->setGuizmoTransform(transform);
                    }
                    else if (light->getType() == LIGHT_SPOT)
                    {
                        const Spot& spot = light->getSpot();

                        Quaternion rotation = Quaternion::FromToRotation(Vector3::UnitZ, spot.Ld);
                        Matrix transform = Matrix::CreateFromQuaternion(rotation);
                        transform.Translation(light->getSpot().Lp);

                        render->setShowGuizmo(true);
                        render->setGuizmoTransform(transform);
                    }

                }
            }

            bool lightDeleted = false;
            if (ImGui::BeginPopupContextItem("Light Menu"))
            {
                if (ImGui::MenuItem("Delete"))
                {
                    scene->removeLight(index);
                    lightDeleted = true;
                    if (selectedIndex == index)
                    {
                        selectionType = SELECTION_NONE;
                        render->setShowGuizmo(false);
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            if (!lightDeleted)
            {
                ++index;
            }
        }
        ImGui::Separator();
        if(ImGui::SmallButton("Add Directional"))
        {
            scene->addLight(Directional(Vector3(0.0f, -1.0f, 0.0f), Vector4::One));
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Add Point"))
        {
            scene->addLight(Point(Vector3(0.0f, 1.0f, 0.0f), 215.0f, Vector4::One));
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Add Spot"))
        {
            scene->addLight(Spot(Vector3(0.0f, -1.0f, 0.0f), 25.0f, Vector3::Zero, cosf(XMConvertToRadians(15.0f)), cosf(XMConvertToRadians(30.0f)), Vector4::One));
        }
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Decals", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const auto& decals = scene->getDecals();
        UINT index = 0;
        for(const auto& decal : decals)
        {
            UINT id = index++;

            ImGui::PushID(id);

            std::string decalName = "Decal " + std::to_string(id);

            bool selected = (id == selectedIndex) && selectionType == SELECTION_DECAL;

            if (selected)
            {
                decal->setTransform(render->getGuizmoTransform());
            }

            if (ImGui::Selectable(decalName.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (selected)
                {
                    selectionType = SELECTION_NONE;
                    render->setShowGuizmo(false);
                }
                else
                {
                    selectedIndex = id;
                    selectionType = SELECTION_DECAL;

                    const Matrix& transform = decal->getTransform();

                    render->setShowGuizmo(true);
                    render->setGuizmoTransform(transform);
                }
            }
            ImGui::PopID();
            ImGui::Separator();
            if (ImGui::SmallButton("Add Decal"))
            {
                scene->addDecal(nullptr, nullptr, Matrix::Identity);
            }
            ImGui::Separator();
        }
    }

    ImGui::End();
}

void ModuleSceneEditor::imGuiDrawProperties()
{
    ImGui::Begin("Viewer Properties");
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);
    ImGui::Checkbox("Show quadtree", &showQuadTree);
    if (showQuadTree)
    {
        ImGui::SliderInt("QuadTree level", (int*)&quadTreeLevel, 0, 10);
    }
    ImGui::Checkbox("Track frustum", &trackFrustum);
    ImGui::Separator();

    if(selectionType == SELECTION_LIGHT)
    { 
        imGuiDrawLightProperties();
    }
    else if(selectionType == SELECTION_DECAL)
    {
        imGuiDrawDecalProperties();
    }

    ImGui::End();   

}

void ModuleSceneEditor::imGuiDrawDecalProperties()
{
    ModuleScene* scene = app->getScene();

    const auto& decal = scene->getDecal(selectedIndex);
    Matrix transform = decal->getTransform();

    if (ImGui::CollapsingHeader("Decal Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool change = false;

        Vector3 scale;
        Quaternion rotation;
        Vector3 position;
        transform.Decompose(scale, rotation, position);
        Vector3 euler = rotation.ToEuler();

        change = ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.1f) || change;

        change = ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&euler), 0.1f) || change;
        change = ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.1f) || change;

        if (change)
        {
            rotation = Quaternion::CreateFromYawPitchRoll(euler);
            transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(transform.Translation());
            decal->setTransform(transform);
        }
    }
}

void ModuleSceneEditor::imGuiDrawLightProperties()
{
    ModuleScene* scene = app->getScene();
    ModuleRender* render = app->getRender();

    const auto& light = scene->getLight(selectedIndex);
    switch (light->getType())
    {
        case LIGHT_DIRECTIONAL:
        {
            Directional dirLight = light->getDirectional();
            if(imGuiDrawDirectionalProperties(dirLight))
            {
                light->setDirectional(dirLight);
            }
            break;
        }
        case LIGHT_POINT:
        {
            Point pointLight = light->getPoint();
            if (imGuiDrawPointProperties(pointLight))
            {
                light->setPoint(pointLight);

                Matrix transform = Matrix::Identity;
                transform.Translation(light->getPoint().Lp);

                render->setGuizmoTransform(transform);
            }

            break;
        }
        case LIGHT_SPOT:
        {
            Spot spotLight = light->getSpot();
            if (imGuiDrawSpotProperties(spotLight))
            {
                light->setSpot(spotLight);

                Quaternion rotation = Quaternion::FromToRotation(Vector3::UnitZ, spotLight.Ld);
                Matrix transform = Matrix::CreateFromQuaternion(rotation);
                transform.Translation(spotLight.Lp);

                render->setGuizmoTransform(transform);
            }

            break;
        }
    }
}

bool ModuleSceneEditor::imGuiDrawDirectionalProperties(Directional &dirLight)
{
    if (ImGui::CollapsingHeader("Directional Light Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool change = imGuiDirection(dirLight.Ld);
        change = ImGui::ColorEdit3("Colour", reinterpret_cast<float*>(&dirLight.Lc)) || change;
        change = ImGui::DragFloat("Intensity", &dirLight.Lc.w, 0.1f, 0.0f, 1000.0f) || change;

        return change;
    }

    return false;
}

bool ModuleSceneEditor::imGuiDrawPointProperties(Point &pointLight)
{
    if (ImGui::CollapsingHeader("Point Light Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool change = ImGui::DragFloat3("Position", reinterpret_cast<float *>(&pointLight.Lp), 0.1f);
        change = ImGui::ColorEdit3("Colour", reinterpret_cast<float *>(&pointLight.Lc)) || change;
        change = ImGui::DragFloat("Intensity", &pointLight.Lc.w, 0.1f, 0.0f, 1000.0f) || change;
        float radius = sqrtf(pointLight.sqRadius);
        if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 1000.0f))
        {
            pointLight.sqRadius = radius * radius;
            change = true;
        }

        return change;
    }

    return false;
}

bool ModuleSceneEditor::imGuiDrawSpotProperties(Spot &spotLight)
{
    if (ImGui::CollapsingHeader("Spot Light Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool change = ImGui::DragFloat3("Position", reinterpret_cast<float *>(&spotLight.Lp), 0.1f);
        change = imGuiDirection(spotLight.Ld) || change;
        change = ImGui::ColorEdit3("Colour", reinterpret_cast<float *>(&spotLight.Lc)) || change;
        change = ImGui::DragFloat("Intensity", &spotLight.Lc.w, 0.1f, 0.0f, 1000.0f) || change;

        float innerConeAngle = acosf(spotLight.inner);
        float outerConeAngle = acosf(spotLight.outer);
        if (ImGui::SliderAngle("Inner Cone Angle", &innerConeAngle, 0.0f, 90.0f))
        {
            spotLight.inner = cosf(innerConeAngle);
            spotLight.outer = std::min(spotLight.outer, spotLight.inner);
            change = true;
        }
        if (ImGui::SliderAngle("Outer Cone Angle", &outerConeAngle, 0.0f, 90.0f))
        {
            spotLight.outer = cosf(outerConeAngle);
            spotLight.inner = std::max(spotLight.inner, spotLight.outer);
            change = true;
        }

        change = ImGui::DragFloat("Radius", &spotLight.sqRadius, 0.1f, 0.1f, 1000.0f) || change;

        ImGui::Separator();
        ImGui::Checkbox("Show as bounding sphere", &showSpotSphere);

        return change;
    }

    return false;
}

void ModuleSceneEditor::renderDebugDrawModel()
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

    _ASSERTE(selectionType == SELECTION_MODEL);
    app->getScene()->getModel(selectedIndex)->enumerateNodes(drawNode, nullptr);
}

void ModuleSceneEditor::renderDebugDrawDecal()
{
    _ASSERTE(selectionType == SELECTION_DECAL);
    std::shared_ptr<const Decal> decal = app->getScene()->getDecal(selectedIndex);

    const Matrix& transform = decal->getTransform();
    Vector3 position = transform.Translation();
    float width = transform.Right().Length();
    float height = transform.Up().Length();
    float depth = transform.Backward().Length();

    dd::box(ddConvert(position), dd::colors::White, width, height, depth);
}

void ModuleSceneEditor::renderDebugDrawLight()
{
    _ASSERTE(selectionType == SELECTION_LIGHT);
    std::shared_ptr<const Light> light = app->getScene()->getLight(selectedIndex);

    switch (light->getType())
    {
        case LIGHT_DIRECTIONAL:
        {
            const float distance = 10.0f;
            const float size = 0.5f;
            const Directional &dirLight = light->getDirectional();
            Vector3 color(dirLight.Lc.x * dirLight.Lc.w, dirLight.Lc.y * dirLight.Lc.w, dirLight.Lc.z * dirLight.Lc.w);
            dd::arrow(ddConvert(-dirLight.Ld * (distance + size)), ddConvert(-dirLight.Ld * distance), ddConvert(color), 0.1f);
            break;
        }
        case LIGHT_POINT:
        {
            const Point &pointLight = light->getPoint();
            Vector3 color(pointLight.Lc.x * pointLight.Lc.w, pointLight.Lc.y * pointLight.Lc.w, pointLight.Lc.z * pointLight.Lc.w);
            dd::sphere(ddConvert(pointLight.Lp), ddConvert(color), sqrtf(pointLight.sqRadius));
            break;
        }
        case LIGHT_SPOT:
        {
            const Spot& spotLight = light->getSpot();

            if (showSpotSphere)
            {
                Vector4 sphere = getBoundingSphere(spotLight);
                Vector3 color(spotLight.Lc.x * spotLight.Lc.w, spotLight.Lc.y * spotLight.Lc.w, spotLight.Lc.z * spotLight.Lc.w);
                dd::sphere(ddConvert(Vector3(sphere.x, sphere.y, sphere.z)), ddConvert(color), sphere.w);
            }

            Vector3 color(spotLight.Lc.x * spotLight.Lc.w, spotLight.Lc.y * spotLight.Lc.w, spotLight.Lc.z * spotLight.Lc.w);
            dd::arrow(ddConvert(spotLight.Lp), ddConvert(spotLight.Lp + spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(color), 0.1f);
            dd::cone(ddConvert(spotLight.Lp), ddConvert(spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(color), sqrtf(spotLight.sqRadius) * tanf(acosf(spotLight.outer)), 0.0f);
            
            break;
        }
    }
}

Json::object ModuleSceneEditor::serialize() const
{
    Json::object skyboxJson;
    Json::object cameraJson;
    Json::object renderJson;
    Json::object sceneJson;
    Json::object editorJson;

    ModuleCamera* camera = app->getCamera();
    ModuleRender* render = app->getRender(); 
    ModuleScene* scene = app->getScene();

    cameraJson["polar"] = camera->getPolar();
    cameraJson["azimuthal"] = camera->getAzimuthal();
    cameraJson["translation"] = serializeVector3(camera->getTranslation());

    editorJson["showAxis"] = showAxis;
    editorJson["showGrid"] = showGrid;
    editorJson["showQuadTree"] = showQuadTree;
    editorJson["trackFrustum"] = trackFrustum;

    skyboxJson["path"] = scene->getSkybox()->getPath();
    sceneJson["skybox"] = skyboxJson;
    
    const auto& models = scene->getModels();
    Json::array modelArray;
    for (const auto& model : models)
    {
        Json::object modelJson;
        modelJson["path"] = model->getPath();
        modelJson["transform"] = serializeMatrix(model->getRootTransform());

        modelArray.push_back(modelJson);
    }

    sceneJson["models"] = modelArray;

    const auto& lights = scene->getLights();
    Json::array lightArray;
    for (const auto& light : lights)
    {
        Json::object lightJson;
        switch (light->getType())
        {
        case LIGHT_DIRECTIONAL:
        {
            const Directional& dirLight = light->getDirectional();
            lightJson["type"] = "directional";
            lightJson["direction"] = serializeVector3(dirLight.Ld);
            lightJson["color"] = serializeVector4(dirLight.Lc);
            break;
        }
        case LIGHT_POINT:
        {
            const Point& pointLight = light->getPoint();
            lightJson["type"] = "point";
            lightJson["position"] = serializeVector3(pointLight.Lp);
            lightJson["color"] = serializeVector4(pointLight.Lc);
            lightJson["radius"] = sqrtf(pointLight.sqRadius);
            break;
        }
        case LIGHT_SPOT:
        {
            const Spot& spotLight = light->getSpot();
            lightJson["type"] = "spot";
            lightJson["position"] = serializeVector3(spotLight.Lp);
            lightJson["direction"] = serializeVector3(spotLight.Ld);
            lightJson["color"] = serializeVector4(spotLight.Lc);
            lightJson["radius"] = sqrtf(spotLight.sqRadius);
            lightJson["innerConeAngle"] = XMConvertToDegrees(acosf(spotLight.inner));
            lightJson["outerConeAngle"] = XMConvertToDegrees(acosf(spotLight.outer));
            break;
        }
        }

        lightArray.push_back(lightJson);
    }

    sceneJson["lights"] = lightArray;

    Json::array decalArray;
    const auto& decals = scene->getDecals();
    for (const auto& decal : decals)
    {
        Json::object decalJson;
        decalJson["colorPath"] = decal->getColorPath();
        decalJson["normalPath"] = decal->getNormalPath();
        decalJson["transform"] = serializeMatrix(decal->getTransform());

        decalArray.push_back(decalJson);
    }

    sceneJson["decals"] = decalArray;

    Json::object obj;

    obj["scene"] = sceneJson;
    obj["camera"] = cameraJson;
    obj["render"] = renderJson;
    obj["editor"] = editorJson;

    return obj;
}

void ModuleSceneEditor::deserialize(const Json& obj) 
{
    ModuleCamera* camera = app->getCamera();
    ModuleRender* render = app->getRender();
    ModuleScene* scene = app->getScene();

    const Json& cameraJson = obj["camera"];
    const Json& renderJson = obj["render"];
    const Json& sceneJson = obj["scene"];
    const Json& editorJson = obj["editor"];

    camera->setPolar(float(cameraJson["polar"].number_value()));
    camera->setAzimuthal(float(cameraJson["azimuthal"].number_value()));
    camera->setTranslation(deserializeVector3(cameraJson["translation"]));

    setShowAxis(editorJson["showAxis"].bool_value());
    setShowGrid(editorJson["showGrid"].bool_value());
    setShowQuadtree(editorJson["showQuadTree"].bool_value());
    setTrackFrustum(editorJson["trackFrustum"].bool_value());

    const Json& skyboxJson = sceneJson["skybox"];
    const Json& modelJson = sceneJson["models"];
    const Json& animationsJson = sceneJson["animations"];
    const Json& lightsJson = sceneJson["lights"];
    const Json& decalsJson = sceneJson["decals"];

    scene->getSkybox()->init(skyboxJson["path"].string_value().c_str(), false);

    scene->clearModels();

    for (const Json& modelItem : modelJson.array_items())
    {
        const std::string& path = modelItem["path"].string_value();
        Matrix transform = deserializeMatrix(modelItem["transform"]);

        UINT modelIndex = scene->addModel(path.c_str());
        scene->getModel(modelIndex)->setRootTransform(transform);
    }

    scene->clearClips();

    for (const Json& animItem : animationsJson.array_items())
    {
        const std::string& path = animItem["path"].string_value();

        scene->addClip(path.c_str());
    }

    scene->clearLights();

    for (const Json& lightItem : lightsJson.array_items())
    {
        const std::string& type = lightItem["type"].string_value();

        if (type == "directional")
        {
            Directional dirLight;
            dirLight.Ld = deserializeVector3(lightItem["direction"]);
            dirLight.Lc = deserializeVector4(lightItem["color"]);

            scene->addLight(dirLight);
        }
        else if (type == "point")
        {
            Point pointLight;
            pointLight.Lp = deserializeVector3(lightItem["position"]);
            pointLight.Lc = deserializeVector4(lightItem["color"]);
            pointLight.sqRadius = float(lightItem["radius"].number_value() * lightItem["radius"].number_value());

            scene->addLight(pointLight);
        }
        else if (type == "spot")
        {
            Spot spotLight;
            spotLight.Lp = deserializeVector3(lightItem["position"]);
            spotLight.Ld = deserializeVector3(lightItem["direction"]);
            spotLight.Lc = deserializeVector4(lightItem["color"]);
            spotLight.sqRadius = float(lightItem["radius"].number_value() * lightItem["radius"].number_value());
            spotLight.inner = cosf(XMConvertToRadians(float(lightItem["innerConeAngle"].number_value())));
            spotLight.outer = cosf(XMConvertToRadians(float(lightItem["outerConeAngle"].number_value())));

            scene->addLight(spotLight);
        }
    }

    scene->clearDecals();

    for (const Json& decalItem : decalsJson.array_items())
    {
        std::string colorPath = decalItem["colorPath"].string_value();
        std::string normalPath = decalItem["normalPath"].string_value();

        Matrix transform = deserializeMatrix(decalItem["transform"]);

        scene->addDecal(colorPath.c_str(), normalPath.c_str(), transform);
    }

}
