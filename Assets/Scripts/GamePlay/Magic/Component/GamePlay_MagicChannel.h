#pragma once
#include <vector>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "../../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"

namespace GamePlay::Magic
{
    // 照射・吐息の魔法のプレハブに付ける。撃っている間センサーに入っている敵へ一定間隔でダメージを入れ、時間が来たら消える
    class MagicChannel final : public Component::ComponentBase,
                               public LifeCycleCallback::IUpdatable,
                               public Physics::Callback::ISensorEnterable,
                               public Physics::Callback::ISensorExitable
    {
    public:
        /** @brief 生成直後に呼ぶ。duration_secs の間 tickInterval_secs ごとに当て、見た目が消えるのを待ってから自分も消える */
        void Begin(const std::weak_ptr<GameObject::IGameObject>& caster,
                   GameCore::Damage::PhysicsPower powerPerTick,
                   float duration_secs,
                   float tickInterval_secs);

    private:
        struct ChannelTarget final
        {
            std::weak_ptr<GameObject::IGameObject> owner;
            std::weak_ptr<GameObject::IGameObject> part;
        };

        void OnUpdate() override;
        void OnTriggerEnter(const Physics::Manifold& manifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void ApplyTick();

        /** @brief 1回当てるたびに当たった所へ出す。無くてもよい */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) hitPrefab_;
        [[serialize(0)]] float hitEffectLifeTime_secs_ = 0.8f;
        /** @brief 当て終わってから消えるまでの時間。見た目の消え際を切らないため */
        [[serialize(0)]] float linger_secs_ = 0.5f;
        /** @brief 1回当てるごとの揺れ。何体に当たっても 1 回分 */
        [[serialize(1)]] float hitShakeIntensity_     = 0.15f;
        [[serialize(1)]] float hitShakeDuration_secs_ = 0.08f;

        std::vector<ChannelTarget> targets_;
        std::weak_ptr<GameObject::IGameObject> caster_;
        GameCore::Damage::PhysicsPower powerPerTick_;
        float remainingDuration_secs_ = 0.0f;
        float tickInterval_secs_      = 0.25f;
        float untilNextTick_secs_     = 0.0f;
        bool  isChanneling_           = false;
        bool  hasBegun_               = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(hitPrefab_));
            archive(CEREAL_NVP(hitEffectLifeTime_secs_));
            archive(CEREAL_NVP(linger_secs_));
            archive(CEREAL_NVP(hitShakeIntensity_));
            archive(CEREAL_NVP(hitShakeDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(hitPrefab_));
            if (version >= 0) archive(CEREAL_NVP(hitEffectLifeTime_secs_));
            if (version >= 0) archive(CEREAL_NVP(linger_secs_));
            if (version >= 1) archive(CEREAL_NVP(hitShakeIntensity_));
            if (version >= 1) archive(CEREAL_NVP(hitShakeDuration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Magic::MagicChannel, 1);
