#include "Enemy_Behaviour_Action_SetStorm.h"

#include "../../../../../../../../../GamePlay/Weather/WeatherService.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::SetStorm::DoTick(const TickContext& context)
    {
        auto* weather = GamePlay::Weather::WeatherService::Instance();
        //NOTE: 天候は演出なので、シーンに WeatherService が無くてもツリーは止めない
        if (!weather)
            return TickStatus::Success;

        //NOTE: Sequenceは毎フレーム子0から再Tickされる。同じ目標のままならRPCも送らない
        if (weather->HasStormTarget(intensity_))
            return TickStatus::Success;

        weather->SetStorm(intensity_, blendSeconds_);

        // 権威側限定Tickなら、Tickしていない他ピアの空も同じように曇らせる
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::SetStormRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, intensity_, blendSeconds_);
        }

        return TickStatus::Success;
    }

    void Action::SetStorm::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("intensity_", intensity_);
        ImGuiHelper::OnDrawInputField("blendSeconds_", blendSeconds_);
    }
}
