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
    // 範囲の魔法と罠のプレハブに付ける。センサーに入っている敵へまとめてダメージを入れて消える
    class MagicBlast final : public Component::ComponentBase,
                             public LifeCycleCallback::IUpdatable,
                             public Physics::Callback::ISensorEnterable,
                             public Physics::Callback::ISensorExitable
    {
    public:
        /**
         * @brief 生成直後に呼ぶ。delay_secs 後に起爆する。
         *        detonateOnEnter_ のプレハブ（罠）は delay_secs を待たず、敵が入った時点で起爆する
         */
        void Arm(const std::weak_ptr<GameObject::IGameObject>& caster,
                 GameCore::Damage::PhysicsPower power,
                 float delay_secs);

    private:
        struct BlastTarget final
        {
            std::weak_ptr<GameObject::IGameObject> owner;
            std::weak_ptr<GameObject::IGameObject> part;
        };

        void OnUpdate() override;
        void OnTriggerEnter(const Physics::Manifold& manifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        [[nodiscard]] bool HasLiveTarget() const;
        void Detonate();

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) detonatePrefab_;
        [[serialize(0)]] float detonateEffectLifeTime_secs_ = 2.0f;
        [[serialize(0)]] bool  detonateOnEnter_ = false;
        [[serialize(1)]] float hitShakeIntensity_     = 0.6f;
        [[serialize(1)]] float hitShakeDuration_secs_ = 0.18f;

        std::vector<BlastTarget> targets_;
        std::weak_ptr<GameObject::IGameObject> caster_;
        GameCore::Damage::PhysicsPower power_;
        float remainingDelay_secs_ = 0.0f;
        bool isArmed_ = false;
        bool hasDetonated_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(detonatePrefab_));
            archive(CEREAL_NVP(detonateEffectLifeTime_secs_));
            archive(CEREAL_NVP(detonateOnEnter_));
            archive(CEREAL_NVP(hitShakeIntensity_));
            archive(CEREAL_NVP(hitShakeDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(detonatePrefab_));
            if (version >= 0) archive(CEREAL_NVP(detonateEffectLifeTime_secs_));
            if (version >= 0) archive(CEREAL_NVP(detonateOnEnter_));
            if (version >= 1) archive(CEREAL_NVP(hitShakeIntensity_));
            if (version >= 1) archive(CEREAL_NVP(hitShakeDuration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Magic::MagicBlast, 1);
