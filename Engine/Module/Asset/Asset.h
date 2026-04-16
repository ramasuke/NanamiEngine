#pragma once

namespace NanamiEngine::Module::Asset
{
    /**
     * Asset関連をまとめたHelper
     */
    class Asset final
    {
    public:
        static bool IsLoadingResource();
        static int GetLoadingResourceCount();
    };
}
