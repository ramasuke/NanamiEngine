#pragma once
#include <vector>
#include <memory>

#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Component/Collider/ColliderBase.h"
#include "../../../../Engine/Module/Physics/ContactCallback/SensorEnterable/ISensorEnterable.h"
#include "../../../../Engine/Module/Physics/ContactCallback/ContactExitable/ISensorExitable.h"
#include "../../Core/Game/DamageContext/Physics/PhysicsContext.h"
#include "../../Core/Game/DamageContext/Physics/PhysicsPower.h"

namespace GameCore
{
    struct IDamageContext;
}

namespace GamePlay
{
    template<typename AttackTargetT>
    class AttackArea : public Component::ComponentBase,
                       public Physics::Callback::ISensorEnterable,
                       public Physics::Callback::ISensorExitable
    {
    public:
        struct AttackTarget final
        {
            explicit AttackTarget(
                const std::weak_ptr<GameObject::IGameObject>& gameObject,
                const std::weak_ptr<AttackTargetT>&           target)
                : gameObject_(gameObject)
                , target_(target)
            {
            }

            [[nodiscard]] bool IsExpired() const { return gameObject_.expired() || target_.expired(); }
            [[nodiscard]] bool IsGameObject(const std::shared_ptr<GameObject::IGameObject>& gameObject) const { return gameObject_.lock() == gameObject; }
            [[nodiscard]] GameObject::IGameObject& GameObject() const { return *gameObject_.lock(); }
            [[nodiscard]] AttackTargetT& Target() { return *target_.lock(); }

        private:
            std::weak_ptr<GameObject::IGameObject> gameObject_;
            std::weak_ptr<AttackTargetT>           target_;
        };

        virtual ~AttackArea() = default;
        void PhysicsAttack(GameObject::IGameObject& fromObject, GameCore::Damage::PhysicsPower damagePower);
        bool TryPhysicsAttack(GameObject::IGameObject& fromObject, GameCore::Damage::PhysicsPower damagePower);
        [[nodiscard]] const std::vector<AttackTarget>& Targets          () const { return attackTargets_;        }
        [[nodiscard]] const int                      & AttackTargetCount() const { return attackTargets_.size(); }

    protected:
        virtual void DoAttack(AttackTarget attackTarget, std::unique_ptr<GameCore::IDamageContext> context) = 0;
        
    private:
        void OnTriggerEnter(const Physics::Manifold&, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        std::vector<AttackTarget> attackTargets_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };

    template <typename AttackTargetT>
    void AttackArea<AttackTargetT>::PhysicsAttack(
        GameObject::IGameObject& fromObject,
        const GameCore::Damage::PhysicsPower damagePower)
    {
        for (auto attackTarget : Targets())
        {
            DoAttack(attackTarget, std::make_unique<GameCore::Damage::PhysicsContext>(fromObject, attackTarget.GameObject(), damagePower));
        }
        // Components().Catch<Component::ColliderBase>().lock()->OnDebugDraw();
    }

    template <typename AttackTargetT>
    bool AttackArea<AttackTargetT>::TryPhysicsAttack(
        GameObject::IGameObject& fromObject,
        const GameCore::Damage::PhysicsPower damagePower)
    {
        PhysicsAttack(fromObject, damagePower);
        return !Targets().empty();
    }

    template <typename AttackTargetT>
    void AttackArea<AttackTargetT>::OnTriggerEnter(
        const Physics::Manifold& maniFold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto target = gameObject->Components().Catch<AttackTargetT>();
        if (target.expired())
            return;

        attackTargets_.emplace_back(gameObject, target);
    }

    template <typename AttackTargetT>
    void AttackArea<AttackTargetT>::OnTriggerExit(
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        std::erase_if(
            attackTargets_,
            [&](const AttackTarget& entry)
            {
                return entry.IsGameObject(gameObject);
            }
        );
    }

    template <typename AttackTargetT>
    void AttackArea<AttackTargetT>::OnDrawGui()
    {
        ImGui::TextUnformatted("Attack Area");

        ImGui::Text(
            "Targets: %d",
            static_cast<int>(attackTargets_.size())
        );

        ImGui::Separator();
    }
}

#define REGISTER_ATTACK_AREA_TYPE(TYPE)                                                     \
CEREAL_CLASS_VERSION(GamePlay::AttackArea<TYPE>, 0);                                        \
CEREAL_REGISTER_TYPE(GamePlay::AttackArea<TYPE>);                                           \
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::AttackArea<TYPE>);
