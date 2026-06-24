#pragma once
#include "GamePlay_Enemy_IAttackProjectile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Npc::Enemy
{
    class AttackProjectile final : public Component::ComponentBase,
                                   public LifeCycleCallback::IAwakable,
                                   public IAttackProjectile
    {
    public:
        void SetDamage(GameCore::Damage::PhysicsPower power) override;
        
    private:
        void OnAwake() override;

        GameCore::Damage::PhysicsPower power_;
        
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