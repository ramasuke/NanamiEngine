#include "Enemy_Behaviour_Action_AttachParticle.h"

#include "../../../../../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::AttachParticle::DoTick(const TickContext& context)
    {
        // プレハブ未設定は「演出無し」として扱う
        if (!particlePrefab_)
            return TickStatus::Success;

        // NOTE: 付いて行く動きは同期しない。序章(シングルプレイ)の演出用
        GamePlay::Spawn::SpawnAttachedPrefab(*particlePrefab_.get(), position_, target_.get(), lifeTime_);
        return TickStatus::Success;
    }

    void Action::AttachParticle::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("position_", position_);
        ImGuiHelper::OnDrawInputField("lifeTime_", lifeTime_);
        ImGuiHelper::OnDrawInputField("target_", target_);
        ImGuiHelper::OnDrawInputField("particlePrefab_", particlePrefab_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::AttachParticle)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::AttachParticle)
#pragma endregion
