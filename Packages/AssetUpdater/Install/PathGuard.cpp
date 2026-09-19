#include "PathGuard.h"

#include <cctype>
#include <string_view>

#include "../Text/Utf8.h"

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        constexpr std::string_view PATH_GUARD_ASSETS_PREFIX   = "Assets/";
        constexpr std::string_view PATH_GUARD_FORBIDDEN_CHARS = "\\:*?\"<>|";

        bool PathGuardIsDeviceName(const std::string_view segment)
        {
            std::string stem(segment.substr(0, segment.find('.')));
            for (char& character : stem)
                character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));

            if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")
                return true;
            return stem.size() == 4 && (stem.starts_with("COM") || stem.starts_with("LPT")) && stem[3] >= '1' && stem[3] <= '9';
        }

        bool PathGuardIsSafeSegment(const std::string_view segment)
        {
            if (segment.empty() || segment == "." || segment == "..")
                return false;
            // Windows は末尾のドットと空白を黙って落とすので、別の名前で同じファイルを指せてしまう
            if (segment.back() == '.' || segment.back() == ' ')
                return false;
            for (const char character : segment)
            {
                if (static_cast<unsigned char>(character) < 0x20)
                    return false;
                if (PATH_GUARD_FORBIDDEN_CHARS.find(character) != std::string_view::npos)
                    return false;
            }
            return !PathGuardIsDeviceName(segment);
        }
    }

    std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& gameRoot, const std::string& manifestPath)
    {
        if (!manifestPath.starts_with(PATH_GUARD_ASSETS_PREFIX))
            return std::nullopt;

        const std::string_view whole(manifestPath);
        std::size_t begin = 0;
        while (true)
        {
            const std::size_t slash = whole.find('/', begin);
            const std::size_t end   = slash == std::string_view::npos ? whole.size() : slash;
            if (!PathGuardIsSafeSegment(whole.substr(begin, end - begin)))
                return std::nullopt;
            if (slash == std::string_view::npos)
                break;
            begin = slash + 1;
        }

        const std::wstring wide = Utf8ToWide(manifestPath);
        if (wide.empty())
            return std::nullopt;

        std::error_code error;
        const std::filesystem::path assetsRoot = std::filesystem::weakly_canonical(gameRoot / L"Assets", error);
        if (error)
            return std::nullopt;
        const std::filesystem::path resolved = std::filesystem::weakly_canonical(gameRoot / std::filesystem::path(wide), error);
        if (error)
            return std::nullopt;

        // 途中にジャンクションなどがあっても Assets/ の外へは出さない
        const std::filesystem::path relative = resolved.lexically_relative(assetsRoot);
        if (relative.empty() || *relative.begin() == L"..")
            return std::nullopt;
        return resolved;
    }
}
