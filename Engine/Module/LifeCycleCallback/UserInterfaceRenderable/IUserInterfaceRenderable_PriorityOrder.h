#pragma once
#include <memory>

#include "IUserInterfaceRenderable.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    struct UiRenderableOrderCompare
    {
        bool operator()(const std::shared_ptr<IUserInterfaceRenderable>& a,
                        const std::shared_ptr<IUserInterfaceRenderable>& b) const
        {
            return a->GetRenderOrder() < b->GetRenderOrder();
        }
    };
}
