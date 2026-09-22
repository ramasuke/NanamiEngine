#pragma once
#include <memory>

#include "vec3.hpp"
#include "gtc/quaternion.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../../Scripts/Core/Game/Npc/Enemy/Type/EnemyKind.h"
#include "../../../Scripts/GamePlay/Npc/Enemy/Hyena/HyenaRepository.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Npc
{
    class BossEnemyBase;
}

namespace NanamiEngine::Module::Asset
{
    constexpr auto ENEMY_FACTORY_EXTENSION_LABEL = ".enemyFactory";

    /**
     * 敵の生成口。どの種別がどのプレハブになるかと、生成後に付く見た目(ボスHPゲージ)を
     * まとめて持つ。敵コンポーネント側はUIのプレハブを一切持たない。
     */
    class EnemyFactory final : public ScriptableObject
    {
    public:
        explicit EnemyFactory(const std::string& contentPath = "");

        /** @brief 種別に対応するプレハブを生成し、種別ごとの後処理まで行う */
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> Summon(
            GameCore::Npc::Enemy::EnemyKind kind,
            const glm::vec3& position,
            const glm::quat& rotation);

        [[nodiscard]] const GamePlay::Npc::Enemy::HyenaRepository& Hyenas() const { return hyenaRepository_; }
        void ClearHyenas() { hyenaRepository_.Clear(); }

    private:
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> PrefabOf(GameCore::Npc::Enemy::EnemyKind kind) const;
        void AttachBossHealthGauge(GameCore::Npc::BossEnemyBase& boss);

        [[serialize(0)]] FIELD(PrefabGameObjectFile) normalBossPrefab_;
        [[serialize(0)]] FIELD(PrefabGameObjectFile) normalPrefab_;
        [[serialize(0)]] FIELD(PrefabGameObjectFile) hyenaPrefab_;
        [[serialize(1)]] FIELD(PrefabGameObjectFile) tyrannosaurusPrefab_;
        /** 全ボス共通のHPゲージUIと、その購読を受け持つ Presenter */
        [[serialize(0)]] FIELD(PrefabGameObjectFile) bossHealthGaugeUiPrefab_;
        [[serialize(0)]] FIELD(PrefabGameObjectFile) bossHealthGaugePresenterPrefab_;

        GamePlay::Npc::Enemy::HyenaRepository hyenaRepository_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(normalBossPrefab_));
            archive(CEREAL_NVP(normalPrefab_));
            archive(CEREAL_NVP(hyenaPrefab_));
            archive(CEREAL_NVP(bossHealthGaugeUiPrefab_));
            archive(CEREAL_NVP(bossHealthGaugePresenterPrefab_));
            archive(CEREAL_NVP(tyrannosaurusPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(normalBossPrefab_));
            if (version >= 0) archive(CEREAL_NVP(normalPrefab_));
            if (version >= 0) archive(CEREAL_NVP(hyenaPrefab_));
            if (version >= 0) archive(CEREAL_NVP(bossHealthGaugeUiPrefab_));
            if (version >= 0) archive(CEREAL_NVP(bossHealthGaugePresenterPrefab_));
            if (version >= 1) archive(CEREAL_NVP(tyrannosaurusPrefab_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(EnemyFactory, ENEMY_FACTORY_EXTENSION_LABEL, "Npc::Enemy")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EnemyFactory, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::EnemyFactory);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::EnemyFactory);
#pragma endregion
