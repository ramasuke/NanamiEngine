#include "GamePlay_MagicSupport.h"

#include "geometric.hpp"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../Core/Game/PlayerAvatar/Item/Effect/IItemEffectTarget.h"
#include "../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "GamePlay_MagicAim.h"

namespace GamePlay::Magic
{
    void ForEachSupportTarget(const glm::vec3& center,
                              const float radius,
                              const std::function<void(GameCore::PlayerAvatar::Item::IItemEffectTarget&)>& apply)
    {
        for (const auto& weakAvatar : GameCore::IPlayerAvatar::PlayerAvatars())
        {
            const auto avatar = weakAvatar.lock();
            if (!avatar)
                continue;

            const auto component = std::dynamic_pointer_cast<Component::ComponentBase>(avatar);
            const auto avatarObject = component ? component->Entity().lock() : nullptr;
            if (!avatarObject || !IsSpellApplicableTarget(*avatarObject))
                continue;

            if (glm::distance(avatar->PlayerTransform().GetWorldPos(), center) > radius)
                continue;

            if (auto* target = dynamic_cast<GameCore::PlayerAvatar::Item::IItemEffectTarget*>(&avatar->PlayerStatus()))
                apply(*target);
        }
    }
}
