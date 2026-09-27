#include "Enemy_Behaviour_Action_ToPlayerDistance.h"

#include <optional>

#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ToPlayerDistance::DoTick(const TickContext& context)
    {
        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos();

        // NOTE: BT は権威側でしか回らないので、ローカルプレイヤーではなく全プレイヤーの最寄りで判定する
        std::optional<float> nearestSq;
        for (const auto& weakPlayer : context.AllPlayer())
        {
            const auto player = weakPlayer.lock();
            if (!player)
                continue;

            const glm::vec3 diff   = player->PlayerTransform().GetWorldPos() - selfPos;
            const float     distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            if (!nearestSq || distSq < *nearestSq)
                nearestSq = distSq;
        }
        if (!nearestSq)
            return TickStatus::Failure;

        bool isInner = *nearestSq <= distance_ * distance_;
        return isInner == isInnerDistance_ ? TickStatus::Success : TickStatus::Failure; 
    }

    void Action::ToPlayerDistance::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("distance_", distance_);
        ImGuiHelper::OnDrawInputField("isInnerDistance_", isInnerDistance_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerDistance, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
