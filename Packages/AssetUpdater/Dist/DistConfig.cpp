#include "DistConfig.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        std::string DistConfigTrimSlashes(std::string url)
        {
            while (!url.empty() && url.back() == '/')
                url.pop_back();
            return url;
        }
    }

    std::string DistConfig::FilesBaseUrl() const
    {
        if (publicBaseUrl.empty())
            return {};
        return DistConfigTrimSlashes(publicBaseUrl) + "/files/";
    }

    std::string DistConfig::ManifestUrl() const
    {
        if (publicBaseUrl.empty())
            return {};
        return DistConfigTrimSlashes(publicBaseUrl) + "/manifest.json";
    }
}
