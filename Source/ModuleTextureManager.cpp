#include "Globals.h"

#include "ModuleTextureManager.h"
#include "Application.h"
#include "ModuleResources.h"

ModuleTextureManager::ModuleTextureManager()
{

}

ModuleTextureManager::~ModuleTextureManager()
{

}

ComPtr<ID3D12Resource> ModuleTextureManager::createTexture(const std::filesystem::path& path, bool defaultSRGB)
{
    auto it = textures.find(path);
    if (it != textures.end())
    {
        return it->second;
    }
    else
    {
        ComPtr<ID3D12Resource> texture = app->getResources()->createTextureFromFile(path, defaultSRGB);
        if (texture)
        {
            textures[path] = texture;
        }

        return texture;
    }
}

void ModuleTextureManager::removeTexture(const std::filesystem::path& path)
{
    auto it = textures.find(path);
    if (it != textures.end())
    {
        textures.erase(it);
    }
}