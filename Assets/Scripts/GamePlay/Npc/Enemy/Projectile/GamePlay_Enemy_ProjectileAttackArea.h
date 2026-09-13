#pragma once
#include <memory>
#include <vector>

#include "GamePlay_Enemy_IAttackProjectile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"

namespace GamePlay::Npc::Enemy
{
    /** センサーに入った ITakableEnemyAttack へ、投射物1つにつき1回だけダメージを与える */
    class AttackProjectile final : public Component::ComponentBase,
                                   public Physics::Callback::ISensorEnterable,
                                   public IAttackProjectile
    {
    public:
        void SetDamage(GameCore::Damage::PhysicsPower power) override;

    private:
        void OnTriggerEnter(const Physics::Manifold& manifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        GameCore::Damage::PhysicsPower power_;
        std::vector<std::weak_ptr<GameObject::IGameObject>> hitObjects_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Npc::Enemy::AttackProjectile, 0)