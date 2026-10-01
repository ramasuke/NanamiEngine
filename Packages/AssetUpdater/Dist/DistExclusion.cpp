#include "DistExclusion.h"

#include <algorithm>
#include <string_view>

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        // exe にコンパイルされるので配る必要が無い
        constexpr std::string_view DIST_EXCLUDED_DIRECTORIES[] = { "assets/scripts" };

        // どの階層でもこの名前のディレクトリの下は配信しない (変換前の原本置き場)
        // WARNING: .blend には作業した PC のユーザー名とフルパスが入る
        constexpr std::string_view DIST_EXCLUDED_DIRECTORY_NAMES[] = { "_source" };

        constexpr std::string_view DIST_EXCLUDED_SUFFIXES[] =
        {
            ".meta",     // 本体エントリに畳むので単独では出さない
            ".fbx",      // モデルの原本。実行時は .mv1 だけ要る
            ".blend",    // 同上。作業した PC のユーザー名とフルパスが入っている
            ".blend1",   // Blender の自動バックアップ
            ".efkproj",  // エフェクトの原本。実行時は .efkefc だけ要る
            ".h",
            ".cpp",
            ".bak",
        };

        constexpr std::string_view DIST_EXCLUDED_NAMES[] = { "desktop.ini", "thumbs.db", ".ds_store" };
    }

    std::string AsciiLower(std::string text)
    {
        std::ranges::transform(text, text.begin(), [](const char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; });
        return text;
    }

    bool IsExcludedFromDistribution(const std::string& relPosix)
    {
        const std::string lowered = AsciiLower(relPosix);
        const size_t      slash   = lowered.rfind('/');
        const std::string_view name = slash == std::string::npos ? std::string_view(lowered) : std::string_view(lowered).substr(slash + 1);

        if (std::ranges::find(DIST_EXCLUDED_NAMES, name) != std::end(DIST_EXCLUDED_NAMES))
            return true;
        for (const std::string_view suffix : DIST_EXCLUDED_SUFFIXES)
        {
            if (lowered.ends_with(suffix))
                return true;
        }

        // NOTE: 最後の要素 (ファイル名) はディレクトリ名として見ない
        if (slash != std::string::npos)
        {
            size_t begin = 0;
            while (begin <= slash)
            {
                const size_t end = lowered.find('/', begin);
                const std::string_view part = std::string_view(lowered).substr(begin, end - begin);
                if (std::ranges::find(DIST_EXCLUDED_DIRECTORY_NAMES, part) != std::end(DIST_EXCLUDED_DIRECTORY_NAMES))
                    return true;
                begin = end + 1;
            }
        }

        for (const std::string_view directory : DIST_EXCLUDED_DIRECTORIES)
        {
            if (lowered == directory || (lowered.starts_with(directory) && lowered.size() > directory.size() && lowered[directory.size()] == '/'))
                return true;
        }
        return false;
    }
}
