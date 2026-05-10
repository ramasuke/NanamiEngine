#pragma once
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
    };
}