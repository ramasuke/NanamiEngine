#pragma once
#include <mutex>
#include <vector>

#include "../ContactedData/Engine_Physics_ContactedData.h"

namespace NanamiEngine::Module::Physics
{
    class SensorEnterGroup final
    {
    public:
        void Reserve(size_t size);

        void Add(const PendingEnter& enter);
        void Clear();
        void Dispatch();
        void RemoveByCollider(const JPH::BodyID& id);

    private:
        std::vector<PendingEnter> pending_;
        // OnContactAddedはJoltのジョブスレッドから同時に呼ばれ得るため、書き込みのみ保護する。
        std::mutex addMutex_;
    };
}
