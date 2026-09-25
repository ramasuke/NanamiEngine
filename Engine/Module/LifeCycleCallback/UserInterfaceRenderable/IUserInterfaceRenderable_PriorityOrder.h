#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "IUserInterfaceRenderable.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    struct NANAMI_API UiRenderableOrderCompare
    {
        bool operator()(const std::shared_ptr<IUserInterfaceRenderable>& a,
                        const std::shared_ptr<IUserInterfaceRenderable>& b) const
        {
            return a->GetRenderOrder() < b->GetRenderOrder();
        }
    };
}
