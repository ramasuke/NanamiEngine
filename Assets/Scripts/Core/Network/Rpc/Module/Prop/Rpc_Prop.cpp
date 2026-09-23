#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Prop/ChargeBreakPillar/GamePlay_ChargeBreakPillar.h"

namespace
{
    // NOTE: 位置のずれはシーンの読み込み誤差程度しかないはずなので、離れすぎた設置物は別物として扱う
    constexpr float MAX_MATCH_DISTANCE = 5.0f;

    template<typename T>
    bool IsSameProp(const T& prop, const glm::vec3& position)
    {
        const glm::vec3 delta = prop.Transform().GetWorldPos() - position;
        return glm::dot(delta, delta) <= MAX_MATCH_DISTANCE * MAX_MATCH_DISTANCE;
    }

    // 設置物の演出RPC: 送り手のピアで壊れた設置物を、他ピアでも同じ位置のものを壊して揃える
    struct PropRpcRegistration
    {
        PropRpcRegistration()
        {
            GameCore::Network::ChargePillarCollapseRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, glm::vec3 position, glm::vec3 fallDirection)
                {
                    if (const auto pillar =GamePlay::Prop::ChargeBreakPillar::FindNear(position); pillar && IsSameProp(*pillar, position))
                        pillar->Collapse(fallDirection);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PropRpcRegistration s_propRpcRegistration;
}
