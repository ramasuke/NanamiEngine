#pragma once
#include <memory>

#include "vec3.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::PlayerAvatar::Factory
{
    [[nodiscard]] std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> SummonSwordManAvatar(
        const std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile>& playerAvatarPrefab
        , const glm::vec3& summonPosition
        , const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& parent);
}
