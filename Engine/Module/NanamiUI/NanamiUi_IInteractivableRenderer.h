#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

namespace NanamiEngine::Module::Asset
{
    class SpriteFile;
}

namespace NanamiEngine::Module::NanamiUi
{
    class NANAMI_API IInteractivableRenderer
    {
    public:
        virtual ~IInteractivableRenderer() = default;

        virtual void SetSprite(const std::weak_ptr<Asset::SpriteFile>& sprite) = 0;
    };
}
