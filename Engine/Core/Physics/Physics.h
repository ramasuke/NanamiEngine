#pragma once

#include <memory>
#include "../JoltPhysics/Jolt/Jolt.h"
#include <../JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <../JoltPhysics/Jolt/Core/JobSystemThreadPool.h>

#include "../../Module/Physics/ContactListener/EngineContactListener.h"

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup;
}

namespace NanamiEngine::Core
{
    constexpr auto GRAVITY_SCALE = -90.8f;
    
    class Physics final
    {
    public:
        Physics();
        ~Physics();

        [[nodiscard]] JPH::PhysicsSystem& GetPhysicsSystem() { return physicsSystem_; }
        void Initialize();
        void Update(float deltaTime);
        void UpdateContactedEvent() const;
        void Shutdown();
        void UnSubscribeEngineCollider(const JPH::BodyID& colliderId) const;
        
        [[nodiscard]] JPH::BodyID CreateCollider(
            const JPH::RefConst<JPH::Shape>& shape,
            const JPH::Vec3& position,
            const JPH::Quat& rotation,
            JPH::EMotionType motionType,
            bool isSensor,
            bool isGravity,
            Module::GameObject::ComponentGroup* components);

    private:
        std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator_;
        std::unique_ptr<JPH::JobSystemThreadPool> jobSystem_;
        std::unique_ptr<Module::Physics::EngineContactListener> contactListener_;
        JPH::PhysicsSystem physicsSystem_;
    };
}
