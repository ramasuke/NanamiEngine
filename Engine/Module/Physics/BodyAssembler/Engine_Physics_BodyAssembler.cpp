#include "Engine_Physics_BodyAssembler.h"

#include <ranges>
#include <string>

#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../../Core/Physics/Physics.h"
#include "../../GameObject/Interface/IGameObject.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Log/NanamiEngine_Module_Log.h"
#include "../Component/Collider/Engine_Physics_ColliderBodyAccess.h"
#include "../Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../GroupFilter/Engine_Physics_RigidBodyGroupFilter.h"
#include "../JoltUtility/Engine_Physics_JoltUtility.h"
#include "../UserData/Engine_Physics_UserData.h"
#include "Jolt/Physics/Body/AllowedDOFs.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "gtc/quaternion.hpp"

namespace NanamiEngine::Module::Physics
{
    namespace
    {
        template <typename T>
        std::weak_ptr<T> BodyAssemblerFindWeak(const T& component)
        {
            for (const auto& weak : component.Components().template Catches<T>())
            {
                if (const auto shared = weak.lock(); shared.get() == &component)
                    return shared;
            }
            return {};
        }

        JPH::EAllowedDOFs BodyAssemblerToAllowedDOFs(const Constraints constraints)
        {
            uint8_t dofs = static_cast<uint8_t>(JPH::EAllowedDOFs::All);

            if (HasConstraint(constraints, Constraints::FreezePosX))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationX);
            if (HasConstraint(constraints, Constraints::FreezePosY))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationY);
            if (HasConstraint(constraints, Constraints::FreezePosZ))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationZ);
            if (HasConstraint(constraints, Constraints::FreezeRotX))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationX);
            if (HasConstraint(constraints, Constraints::FreezeRotY))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationY);
            if (HasConstraint(constraints, Constraints::FreezeRotZ))
                dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationZ);

            return static_cast<JPH::EAllowedDOFs>(dofs);
        }

        void BodyAssemblerApplyFreezeToVelocities(
            const Constraints constraints,
            JPH::Vec3& linearVelocity,
            JPH::Vec3& angularVelocity)
        {
            if (HasConstraint(constraints, Constraints::FreezePosX)) linearVelocity.SetX(0.0f);
            if (HasConstraint(constraints, Constraints::FreezePosY)) linearVelocity.SetY(0.0f);
            if (HasConstraint(constraints, Constraints::FreezePosZ)) linearVelocity.SetZ(0.0f);
            if (HasConstraint(constraints, Constraints::FreezeRotX)) angularVelocity.SetX(0.0f);
            if (HasConstraint(constraints, Constraints::FreezeRotY)) angularVelocity.SetY(0.0f);
            if (HasConstraint(constraints, Constraints::FreezeRotZ)) angularVelocity.SetZ(0.0f);
        }

        std::pair<JPH::Vec3, JPH::Quat> BodyAssemblerWorldOrigin(const GameObject::Transform& transform)
        {
            const glm::quat rotation = glm::normalize(transform.GetWorldRot());
            return {
                ToJPHVec3(transform.GetWorldPos()),
                JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w)
            };
        }

        std::string BodyAssemblerObjectName(const Component::ComponentBase& component)
        {
            const auto gameObject = component.Entity().lock();
            return gameObject ? gameObject->Name() : std::string("(destroyed)");
        }

        /** @brief Jolt は NaN の姿勢を渡されるとその場でクラッシュするので、渡す前に弾いて発生元を記録する */
        bool BodyAssemblerWarnNonFiniteTransform(
            const JPH::Vec3& position,
            const JPH::Quat& rotation,
            const Component::ComponentBase& component,
            bool& warned)
        {
            if (!position.IsNaN() && !rotation.IsNaN())
                return false;

            if (!warned)
            {
                warned = true;
                LogError("Physics: '" + BodyAssemblerObjectName(component) + "' の Transform が NaN になっているため、物理へ反映しませんでした");
            }
            return true;
        }
    }

    BodyAssembler::BodyAssembler(Core::Physics& physics)
        : physics_(physics)
    {
    }

    BodyAssembler::~BodyAssembler() = default;

    void BodyAssembler::Register(const Component::RigidBody& rigidBody)
    {
        if (rigidBodies_.contains(&rigidBody))
            return;

        auto& entry = rigidBodies_[&rigidBody];
        entry.rigidBody = BodyAssemblerFindWeak(rigidBody);
        entry.groupId = nextGroupId_++;
        ownershipDirty_ = true;
        dirty_ = true;
    }

    void BodyAssembler::Unregister(const Component::RigidBody& rigidBody)
    {
        const auto it = rigidBodies_.find(&rigidBody);
        if (it == rigidBodies_.end())
            return;

        DestroyBody(it->second.bodyId);
        rigidBodies_.erase(it);
        ownershipDirty_ = true;
        dirty_ = true;
    }

    void BodyAssembler::MarkDirty(const Component::RigidBody& rigidBody)
    {
        const auto it = rigidBodies_.find(&rigidBody);
        if (it == rigidBodies_.end())
            return;

        it->second.dirty = true;
        dirty_ = true;
    }

    void BodyAssembler::Register(const Component::ColliderBase& collider)
    {
        if (colliders_.contains(&collider))
            return;

        auto& entry = colliders_[&collider];
        entry.collider = BodyAssemblerFindWeak(collider);
        ownershipDirty_ = true;
        dirty_ = true;
    }

    void BodyAssembler::Unregister(const Component::ColliderBase& collider)
    {
        const auto it = colliders_.find(&collider);
        if (it == colliders_.end())
            return;

        DestroyBody(it->second.bodyId);
        if (it->second.attachedShape)
        {
            if (const auto owner = rigidBodies_.find(it->second.owner); owner != rigidBodies_.end())
                owner->second.dirty = true;
        }
        colliders_.erase(it);
        ownershipDirty_ = true;
        dirty_ = true;
    }

    void BodyAssembler::MarkDirty(const Component::ColliderBase& collider)
    {
        const auto it = colliders_.find(&collider);
        if (it == colliders_.end())
            return;

        it->second.dirty = true;
        dirty_ = true;
    }

    void BodyAssembler::Flush()
    {
        if (!dirty_)
            return;
        dirty_ = false;

        RemoveExpiredEntries();

        if (ownershipDirty_)
        {
            ownershipDirty_ = false;
            ResolveOwners();
        }
        ResolveCollisionGroups();

        for (auto& entry : colliders_ | std::views::values)
        {
            if (!entry.dirty)
                continue;

            entry.dirty = false;
            RebuildColliderBody(entry);
        }

        for (auto& entry : rigidBodies_ | std::views::values)
        {
            if (!entry.dirty)
                continue;

            entry.dirty = false;
            RebuildRigidBodyBody(entry);
        }
    }

    void BodyAssembler::RemoveExpiredEntries()
    {
        for (auto it = rigidBodies_.begin(); it != rigidBodies_.end();)
        {
            if (!it->second.rigidBody.expired())
            {
                ++it;
                continue;
            }

            DestroyBody(it->second.bodyId);
            it = rigidBodies_.erase(it);
            ownershipDirty_ = true;
        }

        for (auto it = colliders_.begin(); it != colliders_.end();)
        {
            if (!it->second.collider.expired())
            {
                ++it;
                continue;
            }

            DestroyBody(it->second.bodyId);
            if (it->second.attachedShape)
            {
                if (const auto owner = rigidBodies_.find(it->second.owner); owner != rigidBodies_.end())
                    owner->second.dirty = true;
            }
            it = colliders_.erase(it);
            ownershipDirty_ = true;
        }
    }

    void BodyAssembler::ResolveOwners()
    {
        std::unordered_map<const Component::ColliderBase*, const Component::RigidBody*> resolvedOwners;

        for (auto& [rigidBodyPtr, entry] : rigidBodies_)
        {
            const auto previousColliders = std::move(entry.attachedColliders);
            entry.attachedColliders.clear();

            if (const auto rigidBody = entry.rigidBody.lock())
            {
                if (const auto gameObject = rigidBody->Entity().lock())
                    CollectAttachedColliders(*gameObject, *rigidBodyPtr, entry, resolvedOwners);
            }

            if (previousColliders != entry.attachedColliders)
                entry.dirty = true;
        }

        for (auto& [colliderPtr, entry] : colliders_)
        {
            const auto resolved = resolvedOwners.find(colliderPtr);
            const Component::RigidBody* owner = resolved != resolvedOwners.end() ? resolved->second : nullptr;
            if (owner != entry.owner)
            {
                entry.owner = owner;
                entry.dirty = true;
            }

            if (owner || entry.warnedLegacyMotion)
                continue;

            const auto collider = entry.collider.lock();
            if (!collider || ColliderBodyAccess::LegacyMotionType(*collider) == MotionType::Static)
                continue;

            entry.warnedLegacyMotion = true;
            LogWarning("Collider '" + BodyAssemblerObjectName(*collider) + "': 以前は Kinematic/Dynamic でしたが、RigidBody が無いため Static になりました。RigidBody を追加してください");
        }
    }

    void BodyAssembler::ResolveCollisionGroups()
    {
        for (auto& [rigidBodyPtr, entry] : rigidBodies_)
        {
            const auto collisionGroupId = CollisionGroupIdOf(entry);
            if (collisionGroupId == entry.collisionGroupId)
                continue;

            entry.collisionGroupId = collisionGroupId;
            entry.dirty = true;
            // 付いている Sensor も同じ GroupID で作り直す
            for (auto& colliderEntry : colliders_ | std::views::values)
            {
                if (colliderEntry.owner == rigidBodyPtr)
                    colliderEntry.dirty = true;
            }
        }
    }

    JPH::CollisionGroup::GroupID BodyAssembler::CollisionGroupIdOf(const RigidBodyEntry& entry) const
    {
        const auto rigidBody = entry.rigidBody.lock();
        if (!rigidBody || !rigidBody->IsPartOfParent())
            return entry.groupId;

        const auto gameObject = rigidBody->Entity().lock();
        if (!gameObject)
            return entry.groupId;

        for (auto parent = gameObject->Transform().GetParent(); parent; parent = parent->Transform().GetParent())
        {
            const auto parentRigidBody = parent->Components().Catch<Component::RigidBody>().lock();
            if (!parentRigidBody)
                continue;

            if (const auto parentEntry = rigidBodies_.find(parentRigidBody.get()); parentEntry != rigidBodies_.end())
                return CollisionGroupIdOf(parentEntry->second);
        }
        return entry.groupId;
    }

    void BodyAssembler::CollectAttachedColliders(
        GameObject::IGameObject& gameObject,
        const Component::RigidBody& owner,
        RigidBodyEntry& ownerEntry,
        std::unordered_map<const Component::ColliderBase*, const Component::RigidBody*>& resolvedOwners) const
    {
        for (const auto& weak : gameObject.Components().Catches<Component::ColliderBase>())
        {
            const auto collider = weak.lock();
            if (!collider || !colliders_.contains(collider.get()))
                continue;

            resolvedOwners[collider.get()] = &owner;
            if (!ColliderBodyAccess::IsSensor(*collider))
                ownerEntry.attachedColliders.push_back(collider.get());
        }

        for (const auto& child : gameObject.Transform().GetChildren())
        {
            if (IsRegisteredRigidBodyObject(*child))
                continue;

            CollectAttachedColliders(*child, owner, ownerEntry, resolvedOwners);
        }
    }

    void BodyAssembler::RebuildColliderBody(ColliderEntry& entry)
    {
        DestroyBody(entry.bodyId);
        entry.userData.reset();

        const auto collider = entry.collider.lock();
        if (!collider)
            return;

        const bool isSensor = ColliderBodyAccess::IsSensor(*collider);
        if (entry.owner && !isSensor)
        {
            // RigidBody の Body の一部なので、RigidBody 側を作り直す
            if (const auto owner = rigidBodies_.find(entry.owner); owner != rigidBodies_.end())
                owner->second.dirty = true;
            return;
        }

        entry.attachedShape = nullptr;
        const auto shape = ColliderBodyAccess::CreateShape(*collider);
        if (!shape)
            return;

        JPH::CollisionGroup collisionGroup;
        JPH::EMotionType motionType = JPH::EMotionType::Static;
        if (const auto owner = rigidBodies_.find(entry.owner); owner != rigidBodies_.end())
        {
            collisionGroup = JPH::CollisionGroup(physics_.RigidBodyGroupFilter(), owner->second.collisionGroupId, 0);
            motionType = JPH::EMotionType::Kinematic;
        }

        const auto [position, rotation] = ColliderBodyAccess::WorldTransform(*collider);
        entry.userData = std::make_unique<UserData>(collider->Entity());
        entry.bodyId = physics_.CreateBody(
            shape,
            position,
            rotation,
            motionType,
            1.0f,
            isSensor,
            false,
            ColliderBodyAccess::LayerOf(*collider),
            JPH::EAllowedDOFs::All,
            entry.userData.get(),
            ColliderBodyAccess::Friction(*collider),
            collisionGroup);
    }

    void BodyAssembler::RebuildRigidBodyBody(RigidBodyEntry& entry)
    {
        auto& bodyInterface = physics_.GetPhysicsSystem().GetBodyInterface();

        JPH::Vec3 linearVelocity  = JPH::Vec3::sZero();
        JPH::Vec3 angularVelocity = JPH::Vec3::sZero();
        if (!entry.bodyId.IsInvalid())
        {
            if (bodyInterface.GetMotionType(entry.bodyId) == JPH::EMotionType::Dynamic)
            {
                linearVelocity  = bodyInterface.GetLinearVelocity (entry.bodyId);
                angularVelocity = bodyInterface.GetAngularVelocity(entry.bodyId);
            }
            DestroyBody(entry.bodyId);
        }
        entry.userData.reset();

        const auto rigidBody = entry.rigidBody.lock();
        if (!rigidBody)
            return;

        const auto gameObject = rigidBody->Entity().lock();
        if (!gameObject)
            return;

        const auto [bodyPosition, bodyRotation] = BodyAssemblerWorldOrigin(gameObject->Transform());
        const JPH::Quat inverseBodyRotation = bodyRotation.Conjugated();

        std::vector<const ColliderEntry*> parts;
        const Component::ColliderBase* primary = nullptr;
        for (const auto* colliderPtr : entry.attachedColliders)
        {
            const auto colliderIt = colliders_.find(colliderPtr);
            if (colliderIt == colliders_.end())
                continue;

            auto& colliderEntry = colliderIt->second;
            colliderEntry.attachedShape = nullptr;

            const auto collider = colliderEntry.collider.lock();
            if (!collider)
                continue;

            const auto shape = ColliderBodyAccess::CreateShape(*collider);
            if (!shape)
                continue;

            if (rigidBody->MotionType() == MotionType::Dynamic && shape->GetType() == JPH::EShapeType::Mesh)
            {
                LogError("RigidBody '" + gameObject->Name() + "': Dynamic の RigidBody にメッシュの Collider('" + BodyAssemblerObjectName(*collider) + "')は入れられないため除外しました");
                continue;
            }

            // 形状の位置はモデル原点のズレ補正も含めて各 Collider が決める。ここで Transform から計算し直さない
            const auto [colliderPosition, colliderRotation] = ColliderBodyAccess::WorldTransform(*collider);
            colliderEntry.relativePosition = inverseBodyRotation * (colliderPosition - bodyPosition);
            colliderEntry.relativeRotation = (inverseBodyRotation * colliderRotation).Normalized();
            colliderEntry.attachedShape = shape;
            parts.push_back(&colliderEntry);

            const bool isOnRigidBodyObject = collider->Entity().lock() == gameObject;
            if (!primary || (isOnRigidBodyObject && primary->Entity().lock() != gameObject))
                primary = collider.get();
        }

        if (parts.empty())
        {
            LogWarning("RigidBody '" + gameObject->Name() + "': まとめる Collider(Sensor 以外)がありません");
            return;
        }

        const Layer layer    = ColliderBodyAccess::LayerOf (*primary);
        const float friction = ColliderBodyAccess::Friction(*primary);
        for (const auto* part : parts)
        {
            const auto collider = part->collider.lock();
            if (ColliderBodyAccess::LayerOf(*collider) == layer && ColliderBodyAccess::Friction(*collider) == friction)
                continue;

            LogWarning("RigidBody '" + gameObject->Name() + "': Collider ごとに layer_ / friction_ が違います。'" + BodyAssemblerObjectName(*primary) + "' の値を使います");
            break;
        }

        JPH::RefConst<JPH::Shape> bodyShape;
        if (parts.size() == 1)
        {
            const auto& part = *parts.front();
            if (part.relativePosition.IsNearZero() && part.relativeRotation.IsClose(JPH::Quat::sIdentity()))
            {
                bodyShape = part.attachedShape;
            }
            else
            {
                const auto result = JPH::RotatedTranslatedShapeSettings(part.relativePosition, part.relativeRotation, part.attachedShape).Create();
                if (result.HasError())
                {
                    LogError("RigidBody '" + gameObject->Name() + "': 形状の作成に失敗しました: " + result.GetError().c_str());
                    return;
                }
                bodyShape = result.Get();
            }
        }
        else
        {
            JPH::StaticCompoundShapeSettings settings;
            for (const auto* part : parts)
                settings.AddShape(part->relativePosition, part->relativeRotation, part->attachedShape);

            const auto result = settings.Create();
            if (result.HasError())
            {
                LogError("RigidBody '" + gameObject->Name() + "': 形状の作成に失敗しました: " + result.GetError().c_str());
                return;
            }
            bodyShape = result.Get();
        }

        entry.userData = std::make_unique<UserData>(rigidBody->Entity());
        entry.bodyId = physics_.CreateBody(
            bodyShape,
            bodyPosition,
            bodyRotation,
            static_cast<JPH::EMotionType>(rigidBody->MotionType()),
            rigidBody->Mass(),
            false,
            rigidBody->IsGravity(),
            layer,
            BodyAssemblerToAllowedDOFs(rigidBody->FreezeConstraints()),
            entry.userData.get(),
            friction,
            JPH::CollisionGroup(physics_.RigidBodyGroupFilter(), entry.collisionGroupId, 0));

        if (entry.bodyId.IsInvalid() || bodyInterface.GetMotionType(entry.bodyId) != JPH::EMotionType::Dynamic)
            return;

        BodyAssemblerApplyFreezeToVelocities(rigidBody->FreezeConstraints(), linearVelocity, angularVelocity);
        bodyInterface.SetLinearVelocity (entry.bodyId, linearVelocity );
        bodyInterface.SetAngularVelocity(entry.bodyId, angularVelocity);
    }

    void BodyAssembler::DestroyBody(JPH::BodyID& bodyId) const
    {
        if (bodyId.IsInvalid())
            return;

        auto& bodyInterface = physics_.GetPhysicsSystem().GetBodyInterface();
        physics_.UnSubscribeEngineCollider(bodyId);
        bodyInterface.RemoveBody (bodyId);
        bodyInterface.DestroyBody(bodyId);
        bodyId = JPH::BodyID();
    }

    bool BodyAssembler::IsRegisteredRigidBodyObject(GameObject::IGameObject& gameObject) const
    {
        const auto rigidBody = gameObject.Components().Catch<Component::RigidBody>().lock();
        return rigidBody && rigidBodies_.contains(rigidBody.get());
    }

    void BodyAssembler::PushTransform(const Component::RigidBody& rigidBody) const
    {
        const auto it = rigidBodies_.find(&rigidBody);
        if (it == rigidBodies_.end() || it->second.bodyId.IsInvalid())
            return;

        auto& bodyInterface = physics_.GetPhysicsSystem().GetBodyInterface();
        const JPH::BodyID& bodyId = it->second.bodyId;
        const auto [position, rotation] = BodyAssemblerWorldOrigin(rigidBody.Transform());
        if (BodyAssemblerWarnNonFiniteTransform(position, rotation, rigidBody, it->second.warnedNonFiniteTransform))
            return;

        switch (bodyInterface.GetMotionType(bodyId))
        {
        case JPH::EMotionType::Dynamic:
            bodyInterface.SetPositionAndRotationWhenChanged(bodyId, position, rotation, JPH::EActivation::Activate);
            break;
        case JPH::EMotionType::Kinematic:
            bodyInterface.MoveKinematic(bodyId, position, rotation, Time::DeltaTime());
            break;
        default:
            break;
        }
    }

    void BodyAssembler::PullTransform(const Component::RigidBody& rigidBody) const
    {
        if (rigidBody.MotionType() != MotionType::Dynamic)
            return;

        const auto it = rigidBodies_.find(&rigidBody);
        if (it == rigidBodies_.end() || it->second.bodyId.IsInvalid())
            return;

        // Compound では重心と原点がずれるので、重心ではなく原点の位置を書き戻す
        JPH::RVec3 position;
        JPH::Quat  rotation;
        physics_.GetPhysicsSystem().GetBodyInterface().GetPositionAndRotation(it->second.bodyId, position, rotation);

        auto& transform = rigidBody.Transform();
        transform.SetWorldPos(ToVec3(position));
        transform.SetWorldRot(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
    }

    void BodyAssembler::MoveSensor(const Component::ColliderBase& collider) const
    {
        const auto it = colliders_.find(&collider);
        if (it == colliders_.end() || it->second.bodyId.IsInvalid() || !it->second.owner)
            return;

        const auto [position, rotation] = ColliderBodyAccess::WorldTransform(collider);
        if (BodyAssemblerWarnNonFiniteTransform(position, rotation, collider, it->second.warnedNonFiniteTransform))
            return;

        physics_.GetPhysicsSystem().GetBodyInterface().MoveKinematic(it->second.bodyId, position, rotation, Time::DeltaTime());
    }

    std::optional<JPH::BodyID> BodyAssembler::BodyOf(const Component::RigidBody& rigidBody) const
    {
        const auto it = rigidBodies_.find(&rigidBody);
        if (it == rigidBodies_.end() || it->second.bodyId.IsInvalid())
            return std::nullopt;

        return it->second.bodyId;
    }

    size_t BodyAssembler::AttachedColliderCount(const Component::RigidBody& rigidBody) const
    {
        const auto it = rigidBodies_.find(&rigidBody);
        return it == rigidBodies_.end() ? 0 : it->second.attachedColliders.size();
    }

    std::optional<std::pair<JPH::Vec3, JPH::Quat>> BodyAssembler::OwnerBodyTransform(const ColliderEntry& entry) const
    {
        if (!entry.attachedShape)
            return std::nullopt;

        const auto owner = rigidBodies_.find(entry.owner);
        if (owner == rigidBodies_.end() || owner->second.bodyId.IsInvalid())
            return std::nullopt;

        JPH::RVec3 bodyPosition;
        JPH::Quat  bodyRotation;
        physics_.GetPhysicsSystem().GetBodyInterface().GetPositionAndRotation(owner->second.bodyId, bodyPosition, bodyRotation);

        return std::pair{
            bodyPosition + bodyRotation * entry.relativePosition,
            (bodyRotation * entry.relativeRotation).Normalized()
        };
    }

    std::optional<std::pair<JPH::Vec3, JPH::Quat>> BodyAssembler::ShapeWorldTransform(const Component::ColliderBase& collider) const
    {
        const auto it = colliders_.find(&collider);
        if (it == colliders_.end())
            return std::nullopt;

        if (!it->second.bodyId.IsInvalid())
        {
            JPH::RVec3 position;
            JPH::Quat  rotation;
            physics_.GetPhysicsSystem().GetBodyInterface().GetPositionAndRotation(it->second.bodyId, position, rotation);
            return std::pair{ JPH::Vec3(position), rotation };
        }

        return OwnerBodyTransform(it->second);
    }

    std::optional<std::pair<glm::vec3, glm::vec3>> BodyAssembler::WorldBounds(const Component::ColliderBase& collider) const
    {
        const auto it = colliders_.find(&collider);
        if (it == colliders_.end())
            return std::nullopt;

        if (!it->second.bodyId.IsInvalid())
            return GetWorldSpaceBounds(it->second.bodyId);

        const auto transform = OwnerBodyTransform(it->second);
        if (!transform)
            return std::nullopt;

        const JPH::Shape& shape = *it->second.attachedShape;
        const JPH::Mat44 centerOfMassTransform = JPH::Mat44::sRotationTranslation(transform->second, transform->first).PreTranslated(shape.GetCenterOfMass());
        const JPH::AABox bounds = shape.GetWorldSpaceBounds(centerOfMassTransform, JPH::Vec3::sOne());
        return std::pair{ ToVec3(bounds.mMin), ToVec3(bounds.mMax) };
    }

    std::optional<glm::vec3> BodyAssembler::CenterOfMassPosition(const Component::ColliderBase& collider) const
    {
        const auto it = colliders_.find(&collider);
        if (it == colliders_.end())
            return std::nullopt;

        if (!it->second.bodyId.IsInvalid())
            return GetCenterOfMassPosition(it->second.bodyId);

        const auto transform = OwnerBodyTransform(it->second);
        if (!transform)
            return std::nullopt;

        return ToVec3(transform->first + transform->second * it->second.attachedShape->GetCenterOfMass());
    }

    std::shared_ptr<GameObject::IGameObject> BodyAssembler::FindRigidBodyObject(const Component::ColliderBase& collider)
    {
        for (auto gameObject = collider.Entity().lock(); gameObject; gameObject = gameObject->Transform().GetParent())
        {
            if (!gameObject->Components().Catch<Component::RigidBody>().expired())
                return gameObject;
        }
        return nullptr;
    }
}
