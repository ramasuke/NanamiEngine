#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/Physics/ContactCallback/ICollisionEnterable/Engine_Physics_ICollisionEnterable.h"
#include "../../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"

namespace GamePlay::Magic
{
    // 弾の魔法のプレハブに付ける。飛ばし方と威力は ProjectileSpellEffect が Launch で渡す
    class MagicProjectile final : public Component::ComponentBase,
                                  public LifeCycleCallback::IUpdatable,
                                  public Physics::Callback::ICollisionEnterable
    {
    public:
        /** @brief 生成直後に呼ぶ。撃ち手と仲間のアバターには当たっても弾けない */
        void Launch(const std::weak_ptr<GameObject::IGameObject>& caster,
                    GameCore::Damage::PhysicsPower power,
                    const glm::vec3& velocity,
                    float lifeTime_secs);

    private:
        void OnUpdate() override;
        void OnCollisionEnter(const Physics::Manifold& manifold,
                              const std::shared_ptr<GameObject::IGameObject>& other) override;
        void Impact();

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) impactPrefab_;
        [[serialize(0)]] float impactLifeTime_secs_ = 2.0f;

        std::weak_ptr<GameObject::IGameObject> caster_;
        GameCore::Damage::PhysicsPower power_;
        float remainingLifeTime_secs_ = 0.0f;
        bool isLaunched_ = false;
        bool hasImpacted_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(impactPrefab_));
            archive(CEREAL_NVP(impactLifeTime_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(impactPrefab_));
            if (version >= 0) archive(CEREAL_NVP(impactLifeTime_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Magic::MagicProjectile, 0)
