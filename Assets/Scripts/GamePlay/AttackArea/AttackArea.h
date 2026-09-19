#pragma once
#include <algorithm>
#include <vector>
#include <memory>

#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../../../Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
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
                const std::weak_ptr<AttackTargetT>&           target,
                const std::weak_ptr<GameObject::IGameObject>& part)
                : gameObject_(gameObject)
                , target_(target)
                , parts_{ part }
            {
            }

            [[nodiscard]] bool IsExpired() const { return gameObject_.expired() || target_.expired(); }
            [[nodiscard]] bool IsGameObject(const std::shared_ptr<GameObject::IGameObject>& gameObject) const { return gameObject_.lock() == gameObject; }
            [[nodiscard]] bool HasPart() const { return !parts_.empty(); }
            [[nodiscard]] GameObject::IGameObject& GameObject() const { return *gameObject_.lock(); }
            [[nodiscard]] AttackTargetT& Target() { return *target_.lock(); }

            /**
             * 範囲に入っている部位のうち fromPosition に最も近いものを「当たった部位」とする。
             * 剣が腿と脛に同時に重なっていても1つに決まる。
             */
            [[nodiscard]] std::weak_ptr<GameObject::IGameObject> NearestPart(const glm::vec3& fromPosition) const
            {
                std::shared_ptr<GameObject::IGameObject> nearest;
                float nearestSqrDistance = 0.0f;
                for (const auto& part : parts_)
                {
                    const auto locked = part.lock();
                    if (!locked)
                        continue;

                    const glm::vec3 delta = locked->Transform().GetWorldPos() - fromPosition;
                    const float sqrDistance = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                    if (nearest && sqrDistance >= nearestSqrDistance)
                        continue;

                    nearest             = locked;
                    nearestSqrDistance  = sqrDistance;
                }
                return nearest;
            }

            void AddPart(const std::shared_ptr<GameObject::IGameObject>& part)
            {
                if (std::ranges::none_of(parts_, [&](const std::weak_ptr<GameObject::IGameObject>& p) { return p.lock() == part; }))
                    parts_.emplace_back(part);
            }

            void RemovePart(const std::shared_ptr<GameObject::IGameObject>& part)
            {
                std::erase_if(parts_, [&](const std::weak_ptr<GameObject::IGameObject>& p)
                {
                    const auto locked = p.lock();
                    return !locked || locked == part;
                });
            }

        private:
            std::weak_ptr<GameObject::IGameObject> gameObject_;
            std::weak_ptr<AttackTargetT>           target_;
            // 範囲に入っている Body の GameObject。手足(isPartOfParent_)ごとに出入りするので、全部出た時に対象から外す
            std::vector<std::weak_ptr<GameObject::IGameObject>> parts_;
        };

        virtual ~AttackArea() = default;
        void PhysicsAttack   (GameObject::IGameObject& fromObject, GameCore::Damage::PhysicsPower damagePower);
        bool TryPhysicsAttack(GameObject::IGameObject& fromObject, GameCore::Damage::PhysicsPower damagePower);
        /** @brief 溜め攻撃として当てる。露出中の弱点に入るとスタンを取れる */
        bool TryChargedPhysicsAttack(GameObject::IGameObject& fromObject, GameCore::Damage::PhysicsPower damagePower);
        [[nodiscard]] const std::vector<AttackTarget>& Targets          () const;
        [[nodiscard]] int                              AttackTargetCount() const { return static_cast<int>(attackTargets_.size()); }
        [[nodiscard]] Core::Network::NetworkObjectId   NetworkObjectId  () const { return GetNetworkObjectId(); }
        /** 被弾側判定: 対象がネットワーク上で他ピアの所有物ならダメージを適用しない */
        [[nodiscard]] static bool IsDamageApplicableTarget(GameObject::IGameObject& targetObject);

    protected:
        virtual void DoAttack(AttackTarget attackTarget, std::unique_ptr<GameCore::IDamage> context) = 0;

    private:
        void ApplyPhysicsAttack(GameObject::IGameObject& fromObject, GameCore::Damage::PhysicsPower damagePower, bool isChargedAttack);
        void OnTriggerEnter(const Physics::Manifold&, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        std::vector<AttackTarget> attackTargets_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
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
    void AttackArea<AttackTargetT>::ApplyPhysicsAttack(
        GameObject::IGameObject& fromObject,
        const GameCore::Damage::PhysicsPower damagePower,
        const bool isChargedAttack)
    {
        const glm::vec3 attackPosition = Transform().GetWorldPos();

        for (auto attackTarget : Targets())
        {
            // 被弾側判定: 自分が所有していない(他ピアの)アバターにはダメージを与えない
            if (!IsDamageApplicableTarget(attackTarget.GameObject()))
                continue;

            DoAttack(attackTarget, std::make_unique<GameCore::Damage::Physics>(
                fromObject,
                attackTarget.GameObject(),
                damagePower,
                attackTarget.NearestPart(attackPosition),
                isChargedAttack));
        }
        // Components().Catch<Component::ColliderBase>().lock()->OnDebugDraw();
    }

    template <typename AttackTargetT>
    void AttackArea<AttackTargetT>::PhysicsAttack(
        GameObject::IGameObject& fromObject,
        const GameCore::Damage::PhysicsPower damagePower)
    {
        ApplyPhysicsAttack(fromObject, damagePower, false);
    }

    template <typename AttackTargetT>
    bool AttackArea<AttackTargetT>::IsDamageApplicableTarget(GameObject::IGameObject& targetObject)
    {
        const auto networkGameObject = targetObject.Components().Catch<NanamiEngine::Module::Network::NetworkGameObject>().lock();
        if (!networkGameObject)
            return true;

        const auto targetId = networkGameObject->GetNetworkObjectId();
        if (targetId == Core::Network::NetworkObjectId::Invalid())
            return true;

        const auto* runner = NanamiEngine::Module::Network::NetworkRunnerBase::TryGetInstance();
        if (!runner)
            return true;

        return runner->IsLocallyOwned(targetId);
    }

    template <typename AttackTargetT>
    bool AttackArea<AttackTargetT>::TryPhysicsAttack(
        GameObject::IGameObject& fromObject,
        const GameCore::Damage::PhysicsPower damagePower)
    {
        ApplyPhysicsAttack(fromObject, damagePower, false);
        return !Targets().empty();
    }

    template <typename AttackTargetT>
    bool AttackArea<AttackTargetT>::TryChargedPhysicsAttack(
        GameObject::IGameObject& fromObject,
        const GameCore::Damage::PhysicsPower damagePower)
    {
        ApplyPhysicsAttack(fromObject, damagePower, true);
        return !Targets().empty();
    }

    template <typename AttackTargetT>
    const std::vector<typename AttackArea<AttackTargetT>::AttackTarget>&
    AttackArea<AttackTargetT>::Targets() const
    {
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
        const auto owner  = Physics::FindBodyOwner(gameObject);
        const auto target = owner->Components().Catch<AttackTargetT>();
        if (target.expired())
            return;

        const auto existing = std::ranges::find_if(attackTargets_, [&](const AttackTarget& entry) { return entry.IsGameObject(owner); });
        if (existing != attackTargets_.end())
        {
            existing->AddPart(gameObject);
            return;
        }

        attackTargets_.emplace_back(owner, target, gameObject);
    }

    template <typename AttackTargetT>
    void AttackArea<AttackTargetT>::OnTriggerExit(
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        for (auto& entry : attackTargets_)
            entry.RemovePart(gameObject);

        std::erase_if(
            attackTargets_,
            [](const AttackTarget& entry)
            {
                return !entry.HasPart();
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
