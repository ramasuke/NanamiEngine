#include "Engine_Physics_SensorEnterGroup.h"

#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "../../UserData/Engine_Physics_UserData.h"

namespace NanamiEngine::Module::Physics
{
    void SensorEnterGroup::Reserve(const size_t size)
    {
        pending_.reserve(size);
    }

    void SensorEnterGroup::Add(const PendingEnter& enter)
    {
        pending_.push_back(enter);
    }

    void SensorEnterGroup::Clear()
    {
        pending_.clear();
    }

    void SensorEnterGroup::RemoveByCollider(const JPH::BodyID& id)
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

    void SensorEnterGroup::Dispatch()
    {
        for (const auto& enter : pending_)
        {
            //assert(enter.key_.a_ != enter.key_.b_);
            if (enter.key_.a_ == enter.key_.b_)
                continue;

            auto* sensorData = enter.sensorUserData_;
            auto* otherData  = enter.otherUserData_;

            if (sensorData->IsExpired() || otherData->IsExpired())
                continue;

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

        pending_.clear();
    }
}
