#pragma once
#include <memory>
#include "IUpdatable.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    struct UpdatablePriorityCompare
    {
        bool operator()(const std::shared_ptr<IUpdatable>& a,
                        const std::shared_ptr<IUpdatable>& b) const
        {
            return a->UpdatePriority() < b->UpdatePriority();
        }
    };
}