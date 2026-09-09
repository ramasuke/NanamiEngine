#pragma once
#include <vector>
#include <memory>

#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../../../Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "../../../../Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "../../Core/Game/Damage/Physics/Game_Damage_Physics.h"
#include "../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"

namespace GameCore
{
    struct IDamage;
}

namespace GamePlay
{
    /**
     * センサーコライダーに入っている AttackTargetT へダメージを与える攻撃範囲。
     * NetworkComponent として自身の NetworkObjectId を持ち(子オブジェクトでも Spawn 時に自動付与される)、
     * ダメージは「対象をこのピアが所有している場合」にだけ適用する(被弾側判定)。
     * ネットワーク生成されていない対象(NetworkGameObject 無し / Invalid)には従来通り常に適用する。
     */
    template<typename AttackTargetT>
    class AttackArea : public NanamiEngine::Module::Network::NetworkComponent,
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
        [[nodiscard]] const std::vector<AttackTarget>& Targets          () const;
        [[nodiscard]] int                              AttackTargetCount() const { return static_cast<int>(attackTargets_.size()); }
        /** この攻撃範囲自身の NetworkObjectId(RPC の宛先に使う)。ネットワーク生成されていなければ Invalid() */
        [[nodiscard]] Core::Network::NetworkObjectId   NetworkObjectId  () const { return GetNetworkObjectId(); }

    protected:
        virtual void DoAttack(AttackTarget attackTarget, std::unique_ptr<GameCore::IDamage> context) = 0;

    private:
        void OnTriggerEnter(const Physics::Manifold&, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        /** 被弾側判定: 対象がネットワーク上で他ピアの所有物ならダメージを適用しない */
        [[nodiscard]] static bool IsDamageApplicableTarget(GameObject::IGameObject& targetObject);

        std::vector<AttackTarget> attackTargets_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        // NOTE: version は派生クラス(Enemy::AttackArea / PlayerAttackArea)に登録された値が渡される。
        //       基底を ComponentBase → NetworkComponent に変えた際に両派生クラスとも 2 に揃えた。
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<NetworkComponent>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version < 2) archive(cereal::base_class<ComponentBase>(this));
            else             archive(cereal::base_class<NetworkComponent>(this));
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
            // 被弾側判定: 自分が所有していない(他ピアの)アバターにはダメージを与えない
            if (!IsDamageApplicableTarget(attackTarget.GameObject()))
                continue;

            DoAttack(attackTarget, std::make_unique<GameCore::Damage::Physics>(fromObject, attackTarget.GameObject(), damagePower));
        }
        // Components().Catch<Component::ColliderBase>().lock()->OnDebugDraw();
    }

    template <typename AttackTargetT>
    bool AttackArea<AttackTargetT>::IsDamageApplicableTarget(GameObject::IGameObject& targetObject)
    {
        const auto networkGameObject = targetObject.Components().Catch<NanamiEngine::Module::Network::NetworkGameObject>().lock();
        if (!networkGameObject)
            return true;   // ネットワーク生成されていない対象は従来通り

        const auto targetId = networkGameObject->GetNetworkObjectId();
        if (targetId == Core::Network::NetworkObjectId::Invalid())
            return true;   // ID 未付与(シーン直置き等)も従来通り

        const auto* runner = NanamiEngine::Module::Network::NetworkRunnerBase::TryGetInstance();
        if (!runner)
            return true;   // オフライン

        return targetId.IsOwnerBy(runner->GetPlayerId());
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
    const std::vector<typename AttackArea<AttackTargetT>::AttackTarget>&
    AttackArea<AttackTargetT>::Targets() const
    {
        // const メソッドで消したいなら mutable を使う必要がある
        auto& targets = const_cast<std::vector<AttackTarget>&>(attackTargets_);

        targets.erase(
            std::remove_if(
                targets.begin(),
                targets.end(),
                [](const AttackTarget& t) {
                    return t.IsExpired();
                }),
            targets.end()
        );

        return attackTargets_;
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
        ImGui::Text("NetworkObjectId: %s", GetNetworkObjectId().ToString().c_str());

        ImGui::Separator();
    }
}

#define REGISTER_ATTACK_AREA_TYPE(TYPE)                                                     \
CEREAL_CLASS_VERSION(GamePlay::AttackArea<TYPE>, 2);                                        \
CEREAL_REGISTER_TYPE(GamePlay::AttackArea<TYPE>);                                           \
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::AttackArea<TYPE>);
