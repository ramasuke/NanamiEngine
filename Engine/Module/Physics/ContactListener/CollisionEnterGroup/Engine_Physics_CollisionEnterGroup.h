#pragma once
#include <mutex>
#include <vector>
#include "../ContactedData/Engine_Physics_ContactedData.h"

namespace NanamiEngine::Module::Physics
{
    class CollisionEnterGroup final
    {
    public:
        void Reserve(size_t size);

        void Add(const PendingEnter& enter);
        void Dispatch();

        void RemoveByCollider(const JPH::BodyID& id);

    private:
        std::vector<PendingEnter> pending_;
        // OnContactAddedはJoltのジョブスレッドから同時に呼ばれ得るため、Add()の書き込みのみ保護する。
        // Dispatch/RemoveByColliderはphysics更新完了後にメインスレッドからのみ呼ばれるため不要。
        std::mutex addMutex_;
    };
}