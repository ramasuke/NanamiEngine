#include "Asset.h"

#include "DxLib.h"

namespace NanamiEngine::Module::Asset
{
    bool Asset::IsLoadingResource()
    {
        return GetLoadingResourceCount() > 0;
    }

    int Asset::GetLoadingResourceCount()
    {
        return GetASyncLoadNum();
    }
}
