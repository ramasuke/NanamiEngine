#include "Data_EnemyAttackWarning.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../Scripts/GamePlay/Sound/SoundPlayer.h"
#include "../../../Scripts/GamePlay/Spawn/GamePlay_PrefabSpawner.h"

namespace NanamiEngine::Module::Asset
{
    EnemyAttackWarning::EnemyAttackWarning(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void EnemyAttackWarning::PlayWarning(
        const std::shared_ptr<GameObject::IGameObject>& enemy,
        const std::string& boneName,
        const glm::vec3& boneOffset) const
    {
        if (!enemy)
            return;

        if (const auto prefab = effectPrefab_.get())
            GamePlay::Spawn::SpawnBoneFollowingPrefab(*prefab, enemy, boneName, boneOffset);

        if (const auto sound = sound_.get())
            GamePlay::Sound::SoundPlayer::PlaySe(*sound, enemy->Transform().GetWorldPos());
    }

    void EnemyAttackWarning::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("effectPrefab_", effectPrefab_);
        LibCore::ImGuiHelper::OnDrawInputField("sound_", sound_);
        LibCore::ImGuiHelper::OnDrawInputField("lead_secs_", lead_secs_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(EnemyAttackWarning, ENEMY_ATTACK_WARNING_EXTENSION_LABEL, "Npc::Enemy")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::EnemyAttackWarning, NanamiEngine::Module::ScriptableObject);
#pragma endregion
