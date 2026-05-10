#include "Engine_Physics_SensorExitGroup.h"

#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "../../UserData/Engine_Physics_UserData.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/BodyLockInterface.h"
#include "Jolt/Physics/Body/BodyLockMulti.h"

namespace NanamiEngine::Module::Physics
{
    SensorExitGroup::SensorExitGroup(const JPH::PhysicsSystem& physicsSystem)
        : physicsSystem_(physicsSystem)
    {
    }

    void SensorExitGroup::Reserve(const size_t size)
    {
        pending_.reserve(size);
    }

    void SensorExitGroup::Add(const PendingExit& exit)
    {
        pending_.push_back(exit);
    }

    void SensorExitGroup::Clear()
    {
        pending_.clear();
    }

    void SensorExitGroup::RemoveByCollider(const JPH::BodyID& id)
    {
        pending_.erase(
            std::ranges::remove_if(pending_,
                [&](const PendingExit& e)
                {
                    return e.key_.a_ == id || e.key_.b_ == id;
                }).begin(),
            pending_.end()
        );
    }

    void SensorExitGroup::Dispatch()
    {
        const JPH::BodyLockInterface& lockInterface = physicsSystem_.GetBodyLockInterface();

        for (const auto& exit : pending_)
        {
            if (exit.key_.a_ == exit.key_.b_)
                continue;

            JPH::BodyID ids[2] = { exit.key_.a_, exit.key_.b_ };
            JPH::BodyLockMultiRead lock(lockInterface, ids, 2);

            const JPH::Body* bodyA = lock.GetBody(0);
            const JPH::Body* bodyB = lock.GetBody(1);

            if (!bodyA || !bodyB)
                continue;

            // sensor + rigid のみ通す
            if (bodyA->IsSensor() == bodyB->IsSensor())
                continue;

            const JPH::Body* sensor = bodyA->IsSensor() ? bodyA : bodyB;
            const JPH::Body* other  = bodyA->IsSensor() ? bodyB : bodyA;

            const auto sensorData = reinterpret_cast<UserData*>(sensor->GetUserData());
            const auto otherData  = reinterpret_cast<UserData*>(other->GetUserData());

            if (sensorData->IsExpired() || otherData->IsExpired())
                continue;

            for (const auto& weak : sensorData->Components().Catches<Callback::ISensorExitable>())
            {
                if (const auto exitable = weak.lock())
                {
                    exitable->OnTriggerExit(otherData->Entity().lock());
                }
            }
        }

        pending_.clear();
    }
}