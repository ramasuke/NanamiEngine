#pragma once
#include <memory>
#include "../JoltPhysics/Jolt/Jolt.h"
#include <../JoltPhysics/Jolt/Physics/Body/AllowedDOFs.h>
#include <../JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <../JoltPhysics/Jolt/Core/JobSystemThreadPool.h>

#include "../../Module/Physics/ContactListener/Engine_Physics_ContactListener.h"
#include "../../Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"

namespace NanamiEngine::Module::Physics
{
    struct UserData;
    class BodyAssembler;
    class RigidBodyGroupFilter;
}

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup;
}

namespace NanamiEngine::Core
{
    class Physics final
    {
    public:
        Physics();
        ~Physics();

        [[nodiscard]] JPH::PhysicsSystem& GetPhysicsSystem() { return physicsSystem_; }
        [[nodiscard]] Module::Physics::BodyAssembler& Bodies() const { return *bodyAssembler_; }
        [[nodiscard]] const Module::Physics::RigidBodyGroupFilter* RigidBodyGroupFilter() const;
        void Initialize();
        void Update(float deltaTime);
        void Kill();
        void UnSubscribeEngineCollider(const JPH::BodyID& colliderId) const;

        [[nodiscard]] JPH::BodyID CreateBody(
            const JPH::RefConst<JPH::Shape>& shape,
            const JPH::Vec3& position,
            const JPH::Quat& rotation,
            JPH::EMotionType motionType,
            float mass,
            bool isSensor,
            bool isGravity,
            Module::Physics::Layer layer,
            JPH::EAllowedDOFs allowedDOFs,
            Module::Physics::UserData* userData,
            float friction,
            const JPH::CollisionGroup& collisionGroup);

    private:
        std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator_;
        std::unique_ptr<JPH::JobSystemThreadPool> jobSystem_;
        std::unique_ptr<Module::Physics::EngineContactListener> contactListener_;
        JPH::Ref<Module::Physics::RigidBodyGroupFilter> rigidBodyGroupFilter_;
        JPH::PhysicsSystem physicsSystem_;
        // physicsSystem_ より先に破棄されるよう後ろに置く
        std::unique_ptr<Module::Physics::BodyAssembler> bodyAssembler_;
    };
}
