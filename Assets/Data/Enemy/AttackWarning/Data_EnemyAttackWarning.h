#pragma once
#include <cstdint>
#include <memory>
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../../Scripts/Core/Game/Npc/Enemy/Warning/IEnemyWarningEffectProvider.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto ENEMY_ATTACK_WARNING_EXTENSION_LABEL = ".enemyAttackWarning";

    /**
     * @brief 敵の攻撃予兆 (ボーンに付いて行く閃光 + SE)。金 (通常攻撃) / 赤 (強攻撃) のように種類ごとに1つ作り、
     * PhysicsAttack / ChargeRush の warning_ から共有する
     */
    class EnemyAttackWarning final : public ScriptableObject,
                                     public GameCore::Npc::Enemy::IEnemyWarningEffectProvider
    {
    public:
        explicit EnemyAttackWarning(const std::string& contentPath = "");

        [[nodiscard]] float       WarningLead_secs() const override { return lead_secs_; }
        [[nodiscard]] const Guid& WarningGuid     () const override { return GetGuid(); }

        void PlayWarning(
            const std::shared_ptr<GameObject::IGameObject>& enemy,
            const std::string& boneName,
            const glm::vec3& boneOffset) const override;

    private:
        /** ボーンに付いて行かせるプレハブ。寿命はプレハブ側 (ParticleSystem の Destroy) に任せる */
        [[serialize(0)]] FIELD(PrefabGameObjectFile) effectPrefab_;
        [[serialize(0)]] FIELD(SoundFile)            sound_;
        [[serialize(0)]] float                       lead_secs_ = 0.65f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(effectPrefab_));
            archive(CEREAL_NVP(sound_));
            archive(CEREAL_NVP(lead_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(effectPrefab_));
            if (version >= 0) archive(CEREAL_NVP(sound_));
            if (version >= 0) archive(CEREAL_NVP(lead_secs_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EnemyAttackWarning, 0);
#pragma endregion
