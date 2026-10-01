#include "VersionString.h"

#include <algorithm>
#include <vector>

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        std::vector<unsigned long long> VersionStringSplit(const std::string& version)
        {
            std::vector<unsigned long long> parts;
            unsigned long long value = 0;
            for (const char character : version)
            {
                if (character == '.')
                {
                    parts.push_back(value);
                    value = 0;
                    continue;
                }
                if (character >= '0' && character <= '9')
                    value = value * 10 + static_cast<unsigned long long>(character - '0');
            }
            parts.push_back(value);
            return parts;
        }
    }

    bool IsValidVersionString(const std::string& version)
    {
        return !version.empty() && std::ranges::all_of(version, [](const char c)
        {
            return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '.' || c == '_' || c == '-';
        });
    }

    bool IsOlderVersion(const std::string& left, const std::string& right)
    {
        const std::vector<unsigned long long> leftParts  = VersionStringSplit(left);
        const std::vector<unsigned long long> rightParts = VersionStringSplit(right);

        const std::size_t count = (std::max)(leftParts.size(), rightParts.size());
        for (std::size_t i = 0; i < count; ++i)
        {
            const unsigned long long leftValue  = i < leftParts.size()  ? leftParts[i]  : 0;
            const unsigned long long rightValue = i < rightParts.size() ? rightParts[i] : 0;
            if (leftValue != rightValue)
                return leftValue < rightValue;
        }
        return false;
    }
}
