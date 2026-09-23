#include "Enemy_Behaviour_Action_RadiateProjectile.h"

#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../../GamePlay/Npc/Enemy/Projectile/GamePlay_Enemy_IAttackProjectile.h"
#include "../../../../../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::RadiateProjectile::DoTick(const TickContext& context)
    {
        if (!projectilePrefab_)
            return TickStatus::Failure;

        const glm::quat finalRot  = context.EnemyTransform().GetWorldRot();
        const glm::vec3 spawnPos  = spawnPosition_ .get(context);
        const glm::vec3 targetPos = targetPosition_.get(context);

        const auto projectile = GamePlay::Spawn::SpawnMovingPrefab(
            *projectilePrefab_.get(), spawnPos, finalRot, targetPos, moveSpeed_, isFinishedProjectileDestroy_);

        GamePlay::Npc::Enemy::SetProjectileDamage(projectile, physicsDamage_);

        // 権威側限定Tickなら、他ピアにも同じ軌道・ダメージで投射物を出させる(被弾判定は各ピアが自分の所有アバターに対して行う)。
        // targetPos は権威側の値で固定する(Position::TargetObject が各ピアのローカルプレイヤーを指すのを避ける)
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::SpawnMovingPrefabRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable,
                projectilePrefab_->GetGuid(), spawnPos, finalRot, targetPos, moveSpeed_, isFinishedProjectileDestroy_, physicsDamage_);
        }

        return TickStatus::Success;
    }

    void Action::RadiateProjectile::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("physicsDamage_" , physicsDamage_);
        ImGuiHelper::OnDrawInputField("spawnPosition_" , spawnPosition_);
        ImGuiHelper::OnDrawInputField("targetPosition_", targetPosition_);
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("projectilePrefab_", projectilePrefab_);
        ImGuiHelper::OnDrawInputField("moveFinishedProjectileDestroy_", isFinishedProjectileDestroy_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile)
#pragma endregion
