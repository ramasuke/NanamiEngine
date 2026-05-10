#include "Engine_Physics_CollisionExitGroup.h"

#include "../../../Component/ComponentBase.h"
#include "../../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../ContactCallback/ICollisionExitable/Engine_Physics_ICollisionExitable.h"
#include "../../UserData/Engine_Physics_UserData.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/BodyLockInterface.h"
#include "Jolt/Physics/Body/BodyLockMulti.h"

namespace NanamiEngine::Module::Physics
{
    CollisionExitGroup::CollisionExitGroup(const JPH::PhysicsSystem& physicsSystem)
        : physicsSystem_(physicsSystem)
    {
    }

    void CollisionExitGroup::Reserve(const size_t size)
    {
        pending_.reserve(size);
    }

    void CollisionExitGroup::Add(const PendingExit& exit)
    {
        pending_.push_back(exit);
    }

    void CollisionExitGroup::RemoveByCollider(const JPH::BodyID& id)
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

    void CollisionExitGroup::Dispatch()
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

            // Collisionのみ通す
            if (bodyA->IsSensor() || bodyB->IsSensor())
                continue;

            const auto aData = reinterpret_cast<UserData*>(bodyA->GetUserData());
            const auto bData = reinterpret_cast<UserData*>(bodyB->GetUserData());

            if (aData->IsExpired() || bData->IsExpired())
                continue;

            // A
            for (const auto& weak : aData->Components().Catches<Callback::ICollisionExitable>())
            {
                if (const auto comp = weak.lock())
                {
                    comp->OnCollisionExit(bData->Entity().lock());
                }
            }

            // B
            for (const auto& weak : bData->Components().Catches<Callback::ICollisionExitable>())
            {
                if (const auto comp = weak.lock())
                {
                    comp->OnCollisionExit(aData->Entity().lock());
                }
            }
        }

        pending_.clear();
    }
}