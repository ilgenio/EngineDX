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
        Property<Vector3> scales;                
    };

    struct MorphChannel
    {
        std::unique_ptr<float[]> weights;
        std::unique_ptr<float[]> times;
        UINT timeCount = 0;
        UINT weightCount = 0;
        UINT numTargets = 0;
    };

    std::unordered_map<std::string, Channel> channels;
    std::unordered_map<std::string, MorphChannel> morphChannels;

    float duration = 0.0f;

public:
    AnimationClip();
    ~AnimationClip(); 

    void  load(const char* fileName, int animationIndex);
    void  load(const tinygltf::Model& model, int animationIndex);
    float getDuration() const { return duration; }

    void  getPosRotScale(const std::string& nodeName, float time, std::optional<Vector3>& pos, std::optional<Quaternion>& rot, std::optional<Vector3>& scale) const;
    bool  getMorphWeights(const std::string& nodeName, float time, float* output) const;
    bool  blendMorphWeights(const std::string& nodeName, float time, float* output, float blendLambda) const;
    UINT  getMorphTargetCount(const std::string& nodeName) const;
};