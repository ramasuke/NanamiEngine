#include "EngineContactListener.h"


#include "../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../ContactCallback/SensorEnterable/ISensorEnterable.h"
#include "../../Component/ComponentBase.h"
#include "../ContactCallback/ContactExitable/ISensorExitable.h"
#include "../JoltPhysics/Jolt/Physics/Body/Body.h"
#include "../UserData/Physics_UserData.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/BodyLockInterface.h"
#include "Jolt/Physics/Body/BodyLockMulti.h"

void Physics::EngineContactListener::OnUpdate()
{
    TryTriggerEnter();
    TryTriggerExit ();
}

Physics::EngineContactListener::EngineContactListener(
    const JPH::PhysicsSystem& physicsSystem)
        : physicsSystem_(physicsSystem)
{
    //TODO: pendingする必要がないため削除必須
    pendingEnter_.reserve(1024);
    pendingExit_ .reserve(1024);
}

Physics::EngineContactListener::~EngineContactListener() = default;

void Physics::EngineContactListener::OnContactAdded(
    const JPH::Body& body1,
    const JPH::Body& body2,
    const JPH::ContactManifold& manifold,
    JPH::ContactSettings&)
{
    if (body1.IsSensor() == body2.IsSensor())
        return;


    const JPH::Body& sensor = body1.IsSensor() ? body1 : body2;
    const JPH::Body& other  = body1.IsSensor() ? body2 : body1;
    
    const auto sensorPtr = reinterpret_cast<UserData*>(sensor.GetUserData());
    const auto otherPtr  = reinterpret_cast<UserData*>(other .GetUserData());
    assert(sensorPtr);
    assert(otherPtr);

    pendingEnter_.push_back({
        {sensor.GetID(), other.GetID()},
        {
            { manifold.mWorldSpaceNormal.GetX(),
              manifold.mWorldSpaceNormal.GetY(),
              manifold.mWorldSpaceNormal.GetZ() },
            manifold.mPenetrationDepth
        },
        sensorPtr,
        otherPtr
    });
}

void Physics::EngineContactListener::OnContactRemoved(
    const JPH::SubShapeIDPair& pair)
{
    pendingExit_.push_back({
            {
                .a_ = pair.GetBody1ID(),
               .b_ = pair.GetBody2ID()
            }
    });
}

void Physics::EngineContactListener::UnSubscribeEngineCollider(
    const JPH::BodyID& colliderId)
{
    // pendingEnter_
    pendingEnter_.erase(
        std::ranges::remove_if(pendingEnter_
                               ,
                               [&](const PendingEnter& enter)
                               {
                                   return enter.key_.a_ == colliderId || enter.key_.b_ == colliderId;
                               }).begin(),
        pendingEnter_.end()
    );

    // pendingExit_
    pendingExit_.erase(
        std::ranges::remove_if(pendingExit_
                               ,
                               [&](const PendingExit& exit)
                               {
                                   return exit.key_.a_ == colliderId || exit.key_.b_ == colliderId;
                               }).begin(),
        pendingExit_.end()
    );

    // // activeSensorContacts_
    // for (auto it = activeSensorContacts_.begin(); it != activeSensorContacts_.end(); )
    // {
    //     if (it->first.a_ == colliderId || it->first.b_ == colliderId)
    //         it = activeSensorContacts_.erase(it);
    //     else
    //         ++it;
    // }
}

void Physics::EngineContactListener::TryTriggerEnter()
{
    for (const auto& enter : pendingEnter_)
    {
        assert(enter.key_.a_ != enter.key_.b_);
        if (enter.key_.a_ == enter.key_.b_)
            continue;
        
        // auto it = activeSensorContacts_.find(key_);
        // if (it == activeSensorContacts_.end())
        //     continue;

        auto* sensorData = enter.sensorUserData_;
        auto* otherData  = enter.otherUserData_;

        //NOTE: ここでエラーが発生してかつ、enterのkeyがa, bどちらも同じなら想定外の挙動です。
        //上記の場合はOnContactAdded()の時点でチェックを入れてreturnするようにして下さい。
        if (sensorData->IsExpired() || otherData->IsExpired())
            continue;
        
        // if (sensorComponent->Entity().expired())
            // continue;
        
        for (const auto& weak : sensorData->Components().Catches<Callback::ISensorEnterable>())
        {
            if (const auto sensorEnterable = weak.lock())
            {
                sensorEnterable->OnTriggerEnter(
                    enter.maniFold_,
                    otherData->Entity().lock()
                );
            }
        }
    }
    pendingEnter_.clear();
}

void Physics::EngineContactListener::TryTriggerExit()
{
    const JPH::BodyLockInterface& lockInterface = physicsSystem_.GetBodyLockInterface();
    
    for (const auto& exit : pendingExit_)
    {
        assert(exit.key_.a_ != exit.key_.b_);

        JPH::BodyID ids[2] = { exit.key_.a_, exit.key_.b_ };
        JPH::BodyLockMultiRead lock(lockInterface, ids, 2);

        const JPH::Body* bodyA = lock.GetBody(0);
        const JPH::Body* bodyB = lock.GetBody(1);
    if (bodyA == nullptr || bodyB == nullptr)
            continue;
        
        if (bodyA->IsSensor() == bodyB->IsSensor())
            continue;

        const JPH::Body* sensor = bodyA->IsSensor() ? bodyA : bodyB;
        const JPH::Body* other  = bodyA->IsSensor() ? bodyB : bodyA;
        assert(sensor->GetUserData() != 0);
        assert(other ->GetUserData() != 0);
        
        const auto sensorData = reinterpret_cast<UserData*>(sensor->GetUserData());
        const auto otherData  = reinterpret_cast<UserData*>(other ->GetUserData());
        
        if (sensorData->IsExpired() || otherData->IsExpired())
            continue;
        
        for (const auto& weakSensorExitable : sensorData->Components().Catches<Callback::ISensorExitable>())
        {
            if (const auto& sensorExitable = weakSensorExitable.lock())
            {
                sensorExitable->OnTriggerExit(otherData->Entity().lock());
            }
        }
        // activeSensorContacts_.erase(it);
    }
    pendingExit_.clear();
}