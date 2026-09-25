#include "Enemy_Behaviour_Action_Lightning.h"

#include "../../../../../../../../../GamePlay/Weather/WeatherService.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::Lightning::DoTick(const TickContext& context)
    {
        auto* weather = GamePlay::Weather::WeatherService::Instance();
        //NOTE: 天候は演出なので、シーンに WeatherService が無くてもツリーは止めない
        if (!weather)
            return TickStatus::Success;

        //WARNING: 単発演出。Sequenceの再Tickで毎フレーム落雷しないよう、必ずOnceExecuteの下に置くこと
        weather->Lightning(intensity_, durationSeconds_);

        if (context.IsNetworkAuthority())
        {
            GameCore::Network::LightningRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, intensity_, durationSeconds_);
        }

        return TickStatus::Success;
    }

    void Action::Lightning::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("intensity_", intensity_);
        ImGuiHelper::OnDrawInputField("durationSeconds_", durationSeconds_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::Lightning, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
