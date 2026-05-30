#pragma once
#include <array>
#include <string_view>

namespace NanamiEngine::Module::Component::Particle
{
    enum class PlayMode : int
    {
        Loop = 0,
        Destroy = 1,
    };
    
    constexpr std::array SCENE_TYPES
    {
        PlayMode::Loop,
        PlayMode::Destroy,
    };

    constexpr std::string_view ToString(const PlayMode type)
    {
        switch (type)
        {
        case PlayMode::Loop: return "Loop";
        case PlayMode::Destroy: return "Destroy";
        }
        return "Unknown";
    }
}
