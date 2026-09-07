#pragma once
#include <mutex>
#include <vector>
#include "../ContactedData/Engine_Physics_ContactedData.h"

namespace JPH
{
    class PhysicsSystem;
}

namespace NanamiEngine::Module::Physics
{
    class SensorExitGroup final
    {
    public:
        explicit SensorExitGroup(const JPH::PhysicsSystem& physicsSystem);

        void Reserve(size_t size);
        void Add(const PendingExit& exit);
        void Clear();
        void Dispatch();
        void RemoveByCollider(const JPH::BodyID& id);

    private:
        std::vector<PendingExit> pending_;
        const JPH::PhysicsSystem& physicsSystem_;
        // OnContactRemovedはJoltのジョブスレッドから同時に呼ばれ得るため、Add()の書き込みのみ保護する。
        // Dispatch/RemoveByCollider/Clearはphysics更新完了後にメインスレッドからのみ呼ばれるため不要。
        std::mutex addMutex_;
    };
}