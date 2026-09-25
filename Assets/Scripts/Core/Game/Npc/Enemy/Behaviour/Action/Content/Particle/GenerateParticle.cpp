#include "GenerateParticle.h"

#include "../../../../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"
#include "../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{

    TickStatus Action::GenerateParticle::DoTick(const TickContext& context)
    {
        // プレハブ未設定は「演出無し」として扱う
        if (!particlePrefab_)
            return TickStatus::Success;

        const auto enemyPos = context.EnemyTransform().GetWorldPos();
        const auto enemyRot = context.EnemyTransform().GetWorldRot();

        const glm::vec3 rotatedOffset = enemyRot * offset_;

        const glm::vec3 spawnPos =
            isUseAbsolutePosition_
            ? offset_
            : enemyPos + rotatedOffset;

        GamePlay::Spawn::SpawnPrefab(*particlePrefab_.get(), spawnPos, lifeTime_);

        // 権威側限定Tickなら、他ピアにも同じ位置に同じパーティクルを出させる
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::SpawnPrefabRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable,
                particlePrefab_->GetGuid(), spawnPos, lifeTime_);
        }

        return TickStatus::Success;
    }

    void Action::GenerateParticle::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("offset_", offset_);
        ImGuiHelper::OnDrawInputField("lifeTime_", lifeTime_);
        ImGuiHelper::OnDrawInputField("isUseAbsolutePosition_", isUseAbsolutePosition_);
        ImGuiHelper::OnDrawInputField("particlePrefab_", particlePrefab_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::GenerateParticle, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
