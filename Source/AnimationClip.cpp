#include "Globals.h"

#include "AnimationClip.h"

#include "gltf_utils.h"

namespace
{
    template<typename T>
    void loadProperty(AnimationClip::Property<T> &property, const tinygltf::Model &model, int accessorTime, int accessorValue)
    {
        loadAccessorTyped(property.times, property.count, model, accessorTime);
        loadAccessorTyped(property.values, property.count, model, accessorValue);
    }

    void findIndexAndLambda(float* times, UINT count, float time, UINT &index, float &lambda)
    {
        if (time <= times[0])
        {
            index = 0;
            lambda  = 0.0f;
            return;
        }
        if (time >= times[count - 1])
        {
            index = count - 1;
            lambda = 0.0f;
            return;
        }
        auto it = std::upper_bound(times, times + count, time);
        _ASSERTE(it < times + count);

        index = static_cast<UINT>(it - times - 1);
        lambda = (time - times[index]) / (times[index + 1] - times[index]);
    }

    template<typename T>
    void interpolateProperty(const AnimationClip::Property<T> &property, float time, std::optional<T> &out)
    {
        if(property.count == 0)
        {
            return;
        }

        UINT index;
        float lambda;
        findIndexAndLambda(property.times.get(), property.count, time, index, lambda);  

        T tmpOut;
        T::Lerp(property.values[index], property.values[index + 1], lambda, tmpOut);

        out = tmpOut;
    }

}

AnimationClip::AnimationClip()
{

}

AnimationClip::~AnimationClip()
{

}

void AnimationClip::load(const char* fileName, int animationIndex)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
    if (loadOk)
    {
        load(model, animationIndex);
    }
    else
    {
        LOG("Error loading %s: %s", fileName, error.c_str());
    }
}

void AnimationClip::load(const tinygltf::Model &model, int animationIndex)
{
    const std::string translation("translation");
    const std::string rotation("rotation");

    const tinygltf::Animation &animation = model.animations[animationIndex];
    for (const tinygltf::AnimationChannel &animChannel : animation.channels)
    {
        if (animChannel.target_node >= 0)
        {
            const std::string &channelName = model.nodes[animChannel.target_node].name;

            const tinygltf::AnimationSampler &sampler = animation.samplers[animChannel.sampler];
            if (animChannel.target_path == translation)
            {
                Channel &channel = channels[channelName];

                loadProperty(channel.translations, model, sampler.input, sampler.output);

                if (channel.translations.count > 0)
                {
                    duration = std::max(duration, channel.translations.times[channel.translations.count - 1]);
                }
            }
            else if (animChannel.target_path == rotation)
            {
                Channel &channel = channels[channelName];

                loadProperty(channel.rotations, model, sampler.input, sampler.output);

                if (channel.rotations.count > 0)
                {
                    duration = std::max(duration, channel.rotations.times[channel.rotations.count - 1]);
                }
            }
            else if(animChannel.target_path == "scale")
            {
                Channel& channel = channels[channelName];

                loadProperty(channel.scales, model, sampler.input, sampler.output);

                if (channel.scales.count > 0)
                {
                    duration = std::max(duration, channel.scales.times[channel.scales.count - 1]);
                }

            }
            else if (animChannel.target_path == "weights")
            {
                MorphChannel& channel = morphChannels[channelName];

                loadAccessorTyped(channel.times, channel.timeCount, model, sampler.input);
                loadAccessorTyped(channel.weights, channel.weightCount, model, sampler.output);

                _ASSERTE(channel.weightCount % channel.timeCount == 0);

                channel.numTargets = channel.weightCount / channel.timeCount;

                if (channel.timeCount > 0)
                {
                    duration = std::max(duration, channel.times[channel.timeCount - 1]);
                }
            }
        }
    }
}

void AnimationClip::getPosRotScale(const std::string &nodeName, float time, std::optional<Vector3>& pos, std::optional<Quaternion>& rot, 
                                   std::optional<Vector3>& scale) const
{
    auto it = channels.find(nodeName);
    if (it != channels.end())
    {
        const Channel &channel = it->second;

        interpolateProperty(channel.translations, time, pos);
        interpolateProperty(channel.rotations, time, rot);
        interpolateProperty(channel.scales, time, scale);
    }
}

bool AnimationClip::getMorphWeights(const std::string& nodeName, float time, float* output) const
{
    auto it = morphChannels.find(nodeName);
    if (it != morphChannels.end())
    {
        const MorphChannel& channel = it->second;

        for(UINT i = 0; i < channel.numTargets; ++i)
        {
            UINT index;
            float lambda;
            findIndexAndLambda(channel.times.get(), channel.timeCount, time, index, lambda);

            float* weights = &channel.weights[index * channel.numTargets];
            for(UINT j = 0; j < channel.numTargets; ++j)
            {
                output[j] = weights[j]  * (1.0f - lambda);
            }

            if (lambda > 0.0f)
            {
                float* weights = &channel.weights[(index + 1) * channel.numTargets];
                for (UINT j = 0; j < channel.numTargets; ++j)
                {
                    output[j] += weights[j] * lambda;
                }
            }
        }

        return true;
    }

    return false;
}

bool AnimationClip::blendMorphWeights(const std::string& nodeName, float time, float* output, float blendLambda) const
{
    auto it = morphChannels.find(nodeName);
    if (it != morphChannels.end())
    {
        const MorphChannel& channel = it->second;

        for (UINT i = 0; i < channel.numTargets; ++i)
        {
            UINT index;
            float lambda;
            findIndexAndLambda(channel.times.get(), channel.timeCount, time, index, lambda);

            float* weights = &channel.weights[index * channel.numTargets];
            for (UINT j = 0; j < channel.numTargets; ++j)
            {
                output[j] = output[j] * (1.0f- blendLambda) + (weights[j] * (1.0f - lambda)) * blendLambda;
            }

            if (lambda > 0.0f)
            {
                float* weights = &channel.weights[(index + 1) * channel.numTargets];
                for (UINT j = 0; j < channel.numTargets; ++j)
                {
                    output[j] += (weights[j] * lambda) * blendLambda;
                }
            }
        }

        return true;
    }

    return false;
}

UINT AnimationClip::getMorphTargetCount(const std::string& nodeName) const
{
    auto it = morphChannels.find(nodeName);
    if (it != morphChannels.end())
    {
        return it->second.numTargets;
    }

    return 0;
}



