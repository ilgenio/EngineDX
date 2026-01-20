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

    template<typename T>
    void interpolateProperty(const AnimationClip::Property<T> &property, float time, std::optional<T> &out)
    {
        if(property.count == 0)
        {
            return;
        }

        if (time <= property.times[0])
        {
            out = property.values[0];
            return;
        }

        if (time >= property.times[property.count - 1])
        {
            out = property.values[property.count - 1];
            return;
        }

        auto it = std::upper_bound(&property.times[0], &property.times[0] + property.count, time);
        _ASSERTE(it < &property.times[0] + property.count);

        UINT index = static_cast<UINT>(it - &property.times[0] - 1);
        float t = (time - property.times[index]) / (property.times[index + 1] - property.times[index]);

        T tmpOut;
        T::Lerp(property.values[index], property.values[index + 1], t, tmpOut);

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