#pragma once
#include <functional>

#include "vec3.hpp"

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;
}

namespace GamePlay::Magic
{
    /**
     * @brief center から radius 以内のアバターのうち、この画面が持っているものに apply を呼ぶ。
     *        魔法は全員の画面で実行されるので、近くの仲間にはその仲間の画面で効く
     */
    void ForEachSupportTarget(const glm::vec3& center,
                              float radius,
                              const std::function<void(GameCore::PlayerAvatar::Item::IItemEffectTarget&)>& apply);
}
