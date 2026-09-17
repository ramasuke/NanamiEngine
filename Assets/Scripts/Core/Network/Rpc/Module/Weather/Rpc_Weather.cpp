#include "../../Custom_RpcType.h"
#include "../../../../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Weather/WeatherService.h"

namespace
{
    // 汎用演出RPC: 権威側が変えた天候を、この宛先を持つ他ピアの空にも反映する
    struct WeatherRpcRegistration
    {
        WeatherRpcRegistration()
        {
            GameCore::Network::SetStormRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, float intensity, float blendSeconds)
                {
                    if (auto* weather = GamePlay::Weather::WeatherService::Instance())
                        weather->SetStorm(intensity, blendSeconds);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);

            GameCore::Network::LightningRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, float intensity, float durationSeconds)
                {
                    if (auto* weather = GamePlay::Weather::WeatherService::Instance())
                        weather->Lightning(intensity, durationSeconds);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static WeatherRpcRegistration s_weatherRpcRegistration;
}
