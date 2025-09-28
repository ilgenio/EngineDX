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
    bool interpolateProperty(const AnimationClip::Property<T> &property, float time, T &out)
    {
        if (time <= property.times[0])
        {
            out = property.values[0];
            return true;
        }

        if (time >= property.times[property.count - 1])
        {
            out = property.values[property.count - 1];
            return true;
        }

        for (UINT i = 0; i < property.count - 1; ++i)
        {
            if (time >= property.times[i] && time <= property.times[i + 1])
            {
                float t = (time - property.times[i]) / (property.times[i + 1] - property.times[i]);

                T::Lerp (property.values[i], property.values[i + 1], t, out);

                return true;
            }
        }

        return false;
    }

}

AnimationClip::AnimationClip()
{

}

AnimationClip::~AnimationClip()
{

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
        }
    }
}

bool AnimationClip::getPosRot(const std::string &nodeName, float time, Vector3 &pos, Quaternion &rot) const
{
    auto it = channels.find(nodeName);
    if (it != channels.end())
    {
        const Channel &channel = it->second;

        return interpolateProperty(channel.translations, time, pos) &&
               interpolateProperty(channel.rotations, time, rot);
    }

    return false;
}