#include "EngineContactListener.h"


#include "../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../ContactCallback/SensorEnterable/ISensorEnterable.h"
#include "../../Component/ComponentBase.h"
#include "../ContactCallback/ContactExitable/ISensorExitable.h"
#include "../JoltPhysics/Jolt/Physics/Body/Body.h"

void Physics::EngineContactListener::OnUpdate()
{
    TryTriggerEnter();
    TryTriggerExit ();
}

Physics::EngineContactListener::EngineContactListener()
{
    //TODO: pendingする必要がないため削除必須
    pendingEnter_.reserve(1024);
    pendingExit_ .reserve(1024);
}


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

    const ContactKey key{
        .a_ = sensor.GetID(),
        .b_ = other .GetID()
    };

    activeSensorContacts_[key] = {
        reinterpret_cast<void*>(sensor.GetUserData()),
        reinterpret_cast<void*>(other.GetUserData())
    };

    pendingEnter_.push_back({
        key,
        {
            { manifold.mWorldSpaceNormal.GetX(),
              manifold.mWorldSpaceNormal.GetY(),
              manifold.mWorldSpaceNormal.GetZ() },
            manifold.mPenetrationDepth
        }
    });
}

void Physics::EngineContactListener::OnContactRemoved(
    const JPH::SubShapeIDPair& pair)
{
    pendingExit_.push_back({
        .a_ = pair.GetBody1ID(),
        .b_ = pair.GetBody2ID()
    });
}

void Physics::EngineContactListener::UnSubscribeEngineCollider(const JPH::BodyID& colliderId)
{
    // pendingEnter_
    pendingEnter_.erase(
        std::ranges::remove_if(pendingEnter_
                               ,
                               [&](const PendingEnter& p)
                               {
                                   return p.key_.a_ == colliderId || p.key_.b_ == colliderId;
                               }).begin(),
        pendingEnter_.end()
    );

    // pendingExit_
    pendingExit_.erase(
        std::ranges::remove_if(pendingExit_
                               ,
                               [&](const ContactKey& k)
                               {
                                   return k.a_ == colliderId || k.b_ == colliderId;
                               }).begin(),
        pendingExit_.end()
    );

    // activeSensorContacts_
    for (auto it = activeSensorContacts_.begin(); it != activeSensorContacts_.end(); )
    {
        if (it->first.a_ == colliderId || it->first.b_ == colliderId)
            it = activeSensorContacts_.erase(it);
        else
            ++it;
    }
}

void Physics::EngineContactListener::TryTriggerEnter()
{
    for (const auto& [key_, manifold_] : pendingEnter_)
    {
        auto it = activeSensorContacts_.find(key_);
        if (it == activeSensorContacts_.end())
            continue;

        auto* sensorComponent = reinterpret_cast<GameObject::ComponentGroup*>(it->second.sensorUserData_);
        auto* otherComponent  = reinterpret_cast<GameObject::ComponentGroup*>(it->second.otherUserData_);

        for (const auto& weak : sensorComponent->Catches<Callback::ISensorEnterable>())
        {
            if (const auto s = weak.lock())
            {
                s->OnTriggerEnter(
                    manifold_,
                    otherComponent->Entity().lock()
                );
            }
        }
    }
    pendingEnter_.clear();
}

void Physics::EngineContactListener::TryTriggerExit()
{
    for (const auto& key : pendingExit_)
    {
        auto it = activeSensorContacts_.find(key);
        if (it == activeSensorContacts_.end())
            continue;

        auto* sensorComponent = reinterpret_cast<GameObject::ComponentGroup*>(it->second.sensorUserData_);
        auto* otherComponent  = reinterpret_cast<GameObject::ComponentGroup*>(it->second.otherUserData_ );

        for (const auto& weakSensorExitable : sensorComponent->Catches<Callback::ISensorExitable>())
        {
            if (const auto& sensorExitable = weakSensorExitable.lock())
            {
                sensorExitable->OnTriggerExit(otherComponent->Entity().lock());
            }
        }

        activeSensorContacts_.erase(it);
    }
    pendingExit_.clear();
}