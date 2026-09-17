#pragma once
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "vec3.hpp"
#include "Jolt/Jolt.h"
#include "Jolt/Core/Reference.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/CollisionGroup.h"

namespace JPH
{
    class Shape;
}

namespace NanamiEngine::Core
{
    class Physics;
}

namespace NanamiEngine::Module::Component
{
    class ColliderBase;
    class RigidBody;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Physics
{
    struct UserData;

    /**
     * @brief RigidBody と Collider から Jolt の Body を組み立てる。両方の Component を知っているのはここだけ
     * @note OnAwake では登録するだけで、Body は Flush() で作る。
     *       Flush() を呼ぶのは「Awake フェーズの直後」と「物理ステップの直前」だけ
     */
    class BodyAssembler final
    {
    public:
        explicit BodyAssembler(Core::Physics& physics);
        ~BodyAssembler();
        BodyAssembler(const BodyAssembler&) = delete;
        BodyAssembler& operator=(const BodyAssembler&) = delete;

        void Register  (const Component::RigidBody& rigidBody);
        void Unregister(const Component::RigidBody& rigidBody);
        void MarkDirty (const Component::RigidBody& rigidBody);
        void Register  (const Component::ColliderBase& collider);
        void Unregister(const Component::ColliderBase& collider);
        void MarkDirty (const Component::ColliderBase& collider);

        void Flush();

        void PushTransform(const Component::RigidBody& rigidBody) const;
        void PullTransform(const Component::RigidBody& rigidBody) const;
        // RigidBody に付いている Sensor を、自分の Transform の位置へ動かす
        void MoveSensor(const Component::ColliderBase& collider) const;

        [[nodiscard]] std::optional<JPH::BodyID> BodyOf(const Component::RigidBody& rigidBody) const;
        [[nodiscard]] size_t AttachedColliderCount(const Component::RigidBody& rigidBody) const;
        [[nodiscard]] std::optional<std::pair<glm::vec3, glm::vec3>> WorldBounds(const Component::ColliderBase& collider) const;
        [[nodiscard]] std::optional<glm::vec3> CenterOfMassPosition(const Component::ColliderBase& collider) const;
        [[nodiscard]] std::optional<std::pair<JPH::Vec3, JPH::Quat>> ShapeWorldTransform(const Component::ColliderBase& collider) const;
        // 登録の有無に関係なく、階層をたどって一番近い RigidBody を持つ GameObject を返す(Inspector 表示用)
        [[nodiscard]] static std::shared_ptr<GameObject::IGameObject> FindRigidBodyObject(const Component::ColliderBase& collider);

    private:
        struct RigidBodyEntry
        {
            std::weak_ptr<Component::RigidBody> rigidBody;
            JPH::CollisionGroup::GroupID groupId = JPH::CollisionGroup::cInvalidGroup;
            // Body に実際に使う GroupID。isPartOfParent_ なら親の RigidBody のもの
            JPH::CollisionGroup::GroupID collisionGroupId = JPH::CollisionGroup::cInvalidGroup;
            JPH::BodyID bodyId;
            std::unique_ptr<UserData> userData;
            std::vector<const Component::ColliderBase*> attachedColliders;
            bool dirty = true;
            mutable bool warnedNonFiniteTransform = false;
        };

        struct ColliderEntry
        {
            std::weak_ptr<Component::ColliderBase> collider;
            const Component::RigidBody* owner = nullptr;
            // RigidBody がない Collider と Sensor だけが自分の Body を持つ
            JPH::BodyID bodyId;
            std::unique_ptr<UserData> userData;
            // RigidBody にまとめられている時の形状と、RigidBody 原点からの相対姿勢
            JPH::RefConst<JPH::Shape> attachedShape;
            JPH::Vec3 relativePosition = JPH::Vec3::sZero();
            JPH::Quat relativeRotation = JPH::Quat::sIdentity();
            bool dirty = true;
            bool warnedLegacyMotion = false;
            mutable bool warnedNonFiniteTransform = false;
        };

        void RemoveExpiredEntries();
        void ResolveOwners();
        void ResolveCollisionGroups();
        [[nodiscard]] JPH::CollisionGroup::GroupID CollisionGroupIdOf(const RigidBodyEntry& entry) const;
        void CollectAttachedColliders(GameObject::IGameObject& gameObject, const Component::RigidBody& owner, RigidBodyEntry& ownerEntry, std::unordered_map<const Component::ColliderBase*, const Component::RigidBody*>& resolvedOwners) const;
        void RebuildRigidBodyBody(RigidBodyEntry& entry);
        void RebuildColliderBody(ColliderEntry& entry);
        void DestroyBody(JPH::BodyID& bodyId) const;
        [[nodiscard]] bool IsRegisteredRigidBodyObject(GameObject::IGameObject& gameObject) const;
        [[nodiscard]] std::optional<std::pair<JPH::Vec3, JPH::Quat>> OwnerBodyTransform(const ColliderEntry& entry) const;

        Core::Physics& physics_;
        std::unordered_map<const Component::RigidBody*, RigidBodyEntry> rigidBodies_;
        std::unordered_map<const Component::ColliderBase*, ColliderEntry> colliders_;
        JPH::CollisionGroup::GroupID nextGroupId_ = 0;
        bool ownershipDirty_ = false;
        bool dirty_ = false;
    };
}
