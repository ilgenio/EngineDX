#pragma once

#include "Module.h"
#include <unordered_map>

class ModuleTextureManager : public Module
{
public:
    ModuleTextureManager();
    ~ModuleTextureManager();

    ComPtr<ID3D12Resource> createTexture(const std::filesystem::path& path, bool defaultSRGB = false);
    void removeTexture(const std::filesystem::path& path);
    static std::filesystem::path getNormalizedPath(const std::filesystem::path& path) { return std::filesystem::proximate(std::filesystem::weakly_canonical(path)).make_preferred(); }

private:

    std::unordered_map<std::filesystem::path, ComPtr<ID3D12Resource> > textures;

};