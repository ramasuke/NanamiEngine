#include "Physics.h"

#include <../JoltPhysics/Jolt/RegisterTypes.h>

#include "../../Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../Module/Physics/ContactListener/EngineContactListener.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Sample/SimpleBroadPhaseLayerInterface.h"
#include "Jolt/Core/Factory.h"

namespace NanamiEngine::Core
{
    Physics::Physics()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    Physics::~Physics()
    {
        Kill();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void Physics::Initialize()
    {
        tempAllocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        jobSystem_ = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            std::thread::hardware_concurrency() - 1
        );

        static const SimpleBroadPhaseLayerInterface BROAD_PHASE_LAYER_INTERFACE;

        static const JPH::ObjectLayerPairFilter OBJECT_LAYER_PAIR_FILTER;
        static const JPH::ObjectVsBroadPhaseLayerFilter OBJECT_VS_BROAD_PHASE_LAYER_FILTER;

        physicsSystem_.Init(
            1024, 0, 1024, 1024,
            BROAD_PHASE_LAYER_INTERFACE,
            OBJECT_VS_BROAD_PHASE_LAYER_FILTER,
            OBJECT_LAYER_PAIR_FILTER
        );
        physicsSystem_.SetGravity(JPH::Vec3(0, GRAVITY_SCALE, 0));
        contactListener_ = std::make_unique<Module::Physics::EngineContactListener>();
        physicsSystem_.SetContactListener(contactListener_.get());
    }

    void Physics::Update(const float deltaTime)
    {
        physicsSystem_.Update(deltaTime, IN_COLLISION_STEPS, tempAllocator_.get(), jobSystem_.get());
        contactListener_->OnUpdate();
    }

    void Physics::Kill()
    {
        jobSystem_    .reset();
        tempAllocator_.reset();
        JPH::UnregisterTypes();
    }

    void Physics::UnSubscribeEngineCollider(const JPH::BodyID& colliderId) const
    {
        contactListener_->UnSubscribeEngineCollider(colliderId);
    }

    JPH::BodyID Physics::CreateCollider(
        const JPH::RefConst<JPH::Shape>& shape,
        const JPH::Vec3& position,
        const JPH::Quat& rotation,
        const JPH::EMotionType motionType,
        const float mass,
        const bool isSensor,
        const bool isGravity,
        Module::GameObject::ComponentGroup* components)
    {
        JPH::BodyCreationSettings settings(
            shape,
            JPH::RVec3(position),
            rotation,
            motionType,
            0
        );
        
        JPH::MassProperties mp;
        mp.ScaleToMass(mass);
        settings.mMassPropertiesOverride = mp;
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;

        settings.mIsSensor = isSensor;
        settings.mUserData = reinterpret_cast<JPH::uint64>(components);
        if (!isGravity)
            settings.mGravityFactor = 0.0f;

        return physicsSystem_.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
    }
}
