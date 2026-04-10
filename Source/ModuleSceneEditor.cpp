#include "Globals.h"

#include "ModuleSceneEditor.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleRender.h"

#include "Light.h"
#include "Model.h"

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
    imGuiDrawObjects();
    imGuiDrawProperties();

    switch(selectionType)
    {
    case SELECTION_MODEL:
        renderDebugDrawModel();
        break;
    case SELECTION_LIGHT:
        renderDebugDrawLight();
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
        const auto& lights = scene->getLights();
        UINT index = 0;
        UINT dirIndex = 0;
        UINT pointIndex = 0;
        UINT spotIndex = 0;
        for (const auto& light : lights)
        {
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
            ++index;
        }
    }

    ImGui::End();
}

void ModuleSceneEditor::imGuiDrawProperties()
{
    ImGui::Begin("Viewer Options");
    if(selectionType == SELECTION_LIGHT)
    { 
        imGuiDrawLightProperties();
    }
    ImGui::End();   

}

void ModuleSceneEditor::imGuiDrawLightProperties()
{
    ModuleScene* scene = app->getScene();
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
            }

            break;
        }
        case LIGHT_SPOT:
        {
            Spot spotLight = light->getSpot();
            if (imGuiDrawSpotProperties(spotLight))
            {
                light->setSpot(spotLight);
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
        const Spot &spotLight = light->getSpot();
        Vector3 color(spotLight.Lc.x * spotLight.Lc.w, spotLight.Lc.y * spotLight.Lc.w, spotLight.Lc.z * spotLight.Lc.w);
        dd::arrow(ddConvert(spotLight.Lp), ddConvert(spotLight.Lp + spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(color), 0.1f);
        dd::cone(ddConvert(spotLight.Lp), ddConvert(spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(color), sqrtf(spotLight.sqRadius) * tanf(acosf(spotLight.outer)), 0.0f);
        break;
    }
    }
}

