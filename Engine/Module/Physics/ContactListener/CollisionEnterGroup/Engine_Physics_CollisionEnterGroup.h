#pragma once
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
    };
}