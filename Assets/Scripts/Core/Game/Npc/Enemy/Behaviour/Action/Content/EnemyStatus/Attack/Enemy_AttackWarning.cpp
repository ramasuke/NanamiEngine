#include "Enemy_AttackWarning.h"

#include "../../../TickContext/Enemy_Behaviour_TickContext.h"
#include "../../../../../Warning/IEnemyWarningEffectProvider.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    void FireAttackWarning(
        const TickContext& context,
        const IEnemyWarningEffectProvider* provider,
        const std::string& boneName,
        const glm::vec3& boneOffset)
    {
        if (!provider)
            return;

        provider->PlayWarning(context.EnemyGameObjectPtr(), boneName, boneOffset);
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::PlayAttackWarningRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, provider->WarningGuid(), boneName, boneOffset);
        }
    }
}
