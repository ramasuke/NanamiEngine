#pragma once
#include <functional>

#include "vec3.hpp"

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;
}

namespace GamePlay::Magic
{
    // NOTE: center から radius 以内のアバターのうち、このピアが所有するものに apply を呼ぶ
    void ForEachSupportTarget(const glm::vec3& center,
                              float radius,
                              const std::function<void(GameCore::PlayerAvatar::Item::IItemEffectTarget&)>& apply);
}
