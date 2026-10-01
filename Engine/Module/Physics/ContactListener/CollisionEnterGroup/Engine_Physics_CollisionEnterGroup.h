#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <mutex>
#include <vector>
#include "../ContactedData/Engine_Physics_ContactedData.h"

namespace NanamiEngine::Module::Physics
{
    class NANAMI_API CollisionEnterGroup final
    {
    public:
        void Reserve(size_t size);

        void Add(const PendingEnter& enter);
        void Dispatch();

        void RemoveByCollider(const JPH::BodyID& id);

    private:
        std::vector<PendingEnter> pending_;
        // WARNING: Add() は Jolt のジョブスレッドから同時に呼ばれる (それ以外はメインスレッドのみ)
        std::mutex addMutex_;
    };
}