#include "EnemyFactory.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Scripts/Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"
#include "../../../Scripts/GamePlay/Npc/Enemy/Hyena/GamePlay_Enemy_Hyena.h"
#include "../../../Scripts/GamePlay/Ui/BossHealthGauge/Ui_BossHealthGauge.h"
#include "../../../Scripts/GamePlay/Ui/BossHealthGauge/Ui_BossHealthGaugePresenter.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    EnemyFactory::EnemyFactory(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::weak_ptr<GameObject::IGameObject> EnemyFactory::Summon(
        const GameCore::Npc::Enemy::EnemyKind kind,
        const glm::vec3& position,
        const glm::quat& rotation)
    {
        const auto prefab = PrefabOf(kind);
        if (!prefab)
        {
            LogError(std::string("EnemyFactory: ") + std::string(GameCore::Npc::Enemy::ToString(kind))
                + " のプレハブが設定されていません: " + GetContentPath());
            return {};
        }

        const auto enemyObject = Scene::GameObject::Instantiate(*prefab, position, rotation).lock();
        if (!enemyObject)
            return {};

        switch (kind)
        {
        case GameCore::Npc::Enemy::EnemyKind::NormalBoss:
        case GameCore::Npc::Enemy::EnemyKind::Tyrannosaurus:
        case GameCore::Npc::Enemy::EnemyKind::SkeletonDragon:
            {
                if (const auto boss = enemyObject->Components().Catch<GameCore::Npc::BossEnemyBase>().lock())
                    AttachBossHealthGauge(*boss);
                break;
            }

        case GameCore::Npc::Enemy::EnemyKind::Hyena:
            {
                hyenaRepository_.Add(enemyObject->Components().Catch<GamePlay::Npc::Enemy::Hyena>());
                break;
            }

        case GameCore::Npc::Enemy::EnemyKind::Normal:
        case GameCore::Npc::Enemy::EnemyKind::DesertScorpion:
        case GameCore::Npc::Enemy::EnemyKind::SandWorm:
            {
                break;
            }
        }

        return enemyObject;
    }

    std::shared_ptr<PrefabGameObjectFile> EnemyFactory::PrefabOf(const GameCore::Npc::Enemy::EnemyKind kind) const
    {
        switch (kind)
        {
        case GameCore::Npc::Enemy::EnemyKind::NormalBoss: return normalBossPrefab_.get();
        case GameCore::Npc::Enemy::EnemyKind::Normal:     return normalPrefab_    .get();
        case GameCore::Npc::Enemy::EnemyKind::Hyena:      return hyenaPrefab_     .get();
        case GameCore::Npc::Enemy::EnemyKind::Tyrannosaurus: return tyrannosaurusPrefab_.get();
        case GameCore::Npc::Enemy::EnemyKind::DesertScorpion: return desertScorpionPrefab_.get();
        case GameCore::Npc::Enemy::EnemyKind::SandWorm:       return sandWormPrefab_      .get();
        case GameCore::Npc::Enemy::EnemyKind::SkeletonDragon: return skeletonDragonPrefab_.get();
        }

        return nullptr;
    }

    void EnemyFactory::AttachBossHealthGauge(GameCore::Npc::BossEnemyBase& boss)
    {
        const auto gaugePrefab     = bossHealthGaugeUiPrefab_       .get();
        const auto presenterPrefab = bossHealthGaugePresenterPrefab_.get();
        if (!gaugePrefab || !presenterPrefab)
        {
            LogError("EnemyFactory: ボスHPゲージのプレハブが設定されていません: " + GetContentPath());
            return;
        }

        const auto gaugeObject     = Scene::GameObject::Instantiate(*gaugePrefab    , nullptr).lock();
        const auto presenterObject = Scene::GameObject::Instantiate(*presenterPrefab, nullptr).lock();
        if (!gaugeObject || !presenterObject)
            return;

        const auto gauge     = gaugeObject    ->Components().Catch<GamePlay::Ui::BossHealthGauge>();
        const auto presenter = presenterObject->Components().Catch<GamePlay::Ui::BossHealthGaugePresenter>().lock();
        if (gauge.expired() || !presenter)
        {
            LogError("EnemyFactory: ボスHPゲージのプレハブに BossHealthGauge / BossHealthGaugePresenter がありません");
            return;
        }

        presenter->Initialize(gauge, boss);
        boss.SetHealthGaugePresenter(presenter);
    }

    void EnemyFactory::OnDrawGui()
    {
        ScriptableObject::OnDrawGui();
        ImGuiHelper::OnDrawInputField("normalBossPrefab_", normalBossPrefab_);
        ImGuiHelper::OnDrawInputField("normalPrefab_", normalPrefab_);
        ImGuiHelper::OnDrawInputField("hyenaPrefab_", hyenaPrefab_);
        ImGuiHelper::OnDrawInputField("tyrannosaurusPrefab_", tyrannosaurusPrefab_);
        ImGuiHelper::OnDrawInputField("desertScorpionPrefab_", desertScorpionPrefab_);
        ImGuiHelper::OnDrawInputField("sandWormPrefab_", sandWormPrefab_);
        ImGuiHelper::OnDrawInputField("skeletonDragonPrefab_", skeletonDragonPrefab_);
        ImGuiHelper::OnDrawInputField("bossHealthGaugeUiPrefab_", bossHealthGaugeUiPrefab_);
        ImGuiHelper::OnDrawInputField("bossHealthGaugePresenterPrefab_", bossHealthGaugePresenterPrefab_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(EnemyFactory, ENEMY_FACTORY_EXTENSION_LABEL, "Npc::Enemy")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::EnemyFactory);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::EnemyFactory);
#pragma endregion
