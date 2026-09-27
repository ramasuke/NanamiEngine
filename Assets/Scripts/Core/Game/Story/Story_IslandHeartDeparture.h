#pragma once
#include <memory>
#include <optional>

#include "vec3.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Story
{
    /**
     * @brief 島の心臓が狩り場から抜けたことを敵へ知らせる。心臓に引かれて集まっていた獣は、これを見て散っていく
     * @note 心臓の GameObject が破棄される (シーンを抜ける) と、知らせも消える
     */
    class IslandHeartDeparture final
    {
    public:
        static void Notify(const std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject>& heart, const glm::vec3& position);

        /** @brief 今のシーンで心臓が抜けた位置。抜けていなければ nullopt */
        [[nodiscard]] static std::optional<glm::vec3> DepartedFrom();
    };
}
