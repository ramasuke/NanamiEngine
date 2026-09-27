#include "Enemy_Behaviour_Action_BossSandstorm.h"
#include "../../../../../../../../../GamePlay/Weather/Sandstorm.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::BossSandstorm::DoTick(const TickContext& context)
    {
        // NOTE: Sequence は後ろの枝が終わるまで毎フレーム Tick し直すので、変わるときだけ送る
        if (GamePlay::Weather::Sandstorm::IsSummoned() == isSummon_)
            return TickStatus::Success;

        if (isSummon_) GamePlay::Weather::Sandstorm::BeginSummoned();
        else           GamePlay::Weather::Sandstorm::EndSummoned  ();

        if (context.IsNetworkAuthority())
        {
            GameCore::Network::BossSandstormRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, isSummon_, false);
        }
        return TickStatus::Success;
    }

    void Action::BossSandstorm::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isSummon_", isSummon_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::BossSandstorm, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
