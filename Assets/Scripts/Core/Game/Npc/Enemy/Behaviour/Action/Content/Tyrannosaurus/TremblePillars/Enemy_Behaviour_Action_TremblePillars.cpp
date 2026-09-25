#include "Enemy_Behaviour_Action_TremblePillars.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../../../../../../GamePlay/Prop/ChargeBreakPillar/GamePlay_ChargeBreakPillar.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::TremblePillars::DoTick(const TickContext& context)
    {
        const glm::vec3 center = context.EnemyTransform().GetWorldPos();
        GamePlay::Prop::ChargeBreakPillar::TrembleAll(center, radius_);
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ChargePillarTrembleRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, center, radius_);
        }
        return TickStatus::Success;
    }

    void Action::TremblePillars::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("radius_", radius_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::TremblePillars, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
