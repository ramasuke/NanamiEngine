#include "Engine_Physics_CollisionEnterGroup.h"

#include "../../../Component/ComponentBase.h"
#include "../../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../ContactCallback/ICollisionEnterable/Engine_Physics_ICollisionEnterable.h"
#include "../../UserData/Engine_Physics_UserData.h"

namespace NanamiEngine::Module::Physics
{
    void CollisionEnterGroup::Reserve(const size_t size)
    {
        pending_.reserve(size);
    }

    void CollisionEnterGroup::Add(const PendingEnter& enter)
    {
        pending_.push_back(enter);
    }

    void CollisionEnterGroup::RemoveByCollider(const JPH::BodyID& id)
    {
        pending_.erase(
            std::ranges::remove_if(pending_,
                [&](const PendingEnter& e)
                {
                    return e.key_.a_ == id || e.key_.b_ == id;
                }).begin(),
            pending_.end()
        );
    }

    void CollisionEnterGroup::Dispatch()
    {
        for (const auto& enter : pending_)
        {
            if (enter.key_.a_ == enter.key_.b_)
                continue;

            auto* aData = enter.sensorUserData_;
            auto* bData = enter.otherUserData_;

            if (aData->IsExpired() || bData->IsExpired())
                continue;

            // A側
            for (const auto& weak : aData->Components().Catches<Callback::ICollisionEnterable>())
            {
                if (const auto comp = weak.lock())
                {
                    comp->OnCollisionEnter(
                        enter.maniFold_,
                        bData->Entity().lock()
                    );
                }
            }

            // B側
            for (const auto& weak : bData->Components().Catches<Callback::ICollisionEnterable>())
            {
                if (const auto comp = weak.lock())
                {
                    comp->OnCollisionEnter(
                        enter.maniFold_,
                        aData->Entity().lock()
                    );
                }
            }
        }

        pending_.clear();
    }
}
