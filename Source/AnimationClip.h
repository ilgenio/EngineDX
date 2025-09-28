#pragma once

namespace tinygltf { class Model; }

class AnimationClip
{
    template<typename T>
    struct Property
    {
        std::unique_ptr<T[]> values;
        std::unique_ptr<float[]> times;
        UINT count = 0;
    };

    struct Channel
    {
        Property<Vector3> translations;
        Property<Quaternion> rotations;
    };

    std::unordered_map<std::string, Channel> channels;

    float duration = 0.0f;

public:
    AnimationClip();
    ~AnimationClip(); 

    void  load(const tinygltf::Model& model, int animationIndex);
    float getDuration() const { return duration; }

    bool  getPosRot(const std::string& nodeName, float time, Vector3& pos, Quaternion& rot) const;
};