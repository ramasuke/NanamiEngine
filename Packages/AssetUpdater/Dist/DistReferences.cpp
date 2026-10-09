#include "DistReferences.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string_view>
#include <unordered_set>

#include <windows.h>

#include "DistExclusion.h"
#include "DistHashCache.h"
#include "DistScan.h"
#include "../Text/Utf8.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        // NOTE: INFO チャンクがまだ素の文字列リストだった最後の ExporterVersion
        constexpr std::int32_t     DIST_EFKEFC_STRING_LISTS_MAX_VERSION = 1610;
        constexpr std::int32_t     DIST_EFKEFC_MAX_COUNT                = 4096;
        constexpr std::wstring_view DIST_EFKEFC_ASSET_EXTENSIONS[] = { L"png", L"jpg", L"jpeg", L"bmp", L"tga", L"dds", L"efkmodel", L"efkmat", L"efkcurve", L"wav" };

        constexpr char           DIST_MV1_MAGIC[]      = "MV11";
        constexpr std::size_t    DIST_MV1_HEADER_BYTES = 9;
        constexpr std::size_t    DIST_MV1_MIN_MATCH    = 4;
        constexpr std::size_t    DIST_MV1_MAX_PREFIX   = 259;
        constexpr std::size_t    DIST_MV1_MAX_RESERVE_BYTES = 256u << 20;
        // NOTE: テクスチャ拡張子を名前順に並べたもの (正規表現の選択肢の順)
        constexpr std::string_view DIST_MV1_TEXTURE_EXTENSIONS[] = { "bmp", "dds", "jpeg", "jpg", "png", "tga" };

        constexpr std::string_view DIST_REFERRING_SUFFIXES[] = { ".efkefc", ".mv1" };

        struct DistRefsError {};

        std::uint32_t DistRefsReadU32(const std::string& data, const std::size_t offset)
        {
            if (offset + 4 > data.size())
                throw DistRefsError{};
            std::uint32_t value = 0;
            std::memcpy(&value, data.data() + offset, 4);
            return value;
        }

        std::int32_t DistRefsReadI32(const std::string& data, const std::size_t offset)
        {
            return static_cast<std::int32_t>(DistRefsReadU32(data, offset));
        }

        // NOTE: タグごとの最初のチャンク。ファイル末尾を越えるチャンクがあれば壊れている
        std::optional<std::string> DistRefsEfkefcChunk(const std::string& data, const std::string_view tag)
        {
            if (data.size() < 8 || data.compare(0, 4, "EFKE") != 0)
                throw DistRefsError{};

            std::optional<std::string> found;
            std::size_t offset = 8;
            while (offset + 8 <= data.size())
            {
                const std::string_view chunkTag(data.data() + offset, 4);
                const std::uint32_t    size = DistRefsReadU32(data, offset + 4);
                offset += 8;
                if (offset + size > data.size())
                    throw DistRefsError{};
                if (!found && chunkTag == tag)
                    found = data.substr(offset, size);
                offset += size;
            }
            return found;
        }

        std::wstring DistRefsUtf16(const std::string& bytes, const std::size_t offset, const std::size_t count)
        {
            std::wstring text(count, L'\0');
            std::memcpy(text.data(), bytes.data() + offset, count * sizeof(wchar_t));
            return text;
        }

        // NOTE: 不正なサロゲートがあれば投げる
        std::string DistRefsWideToUtf8Strict(const std::wstring& wide)
        {
            if (wide.empty())
                return {};
            const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
            if (length <= 0)
                throw DistRefsError{};
            std::string text(static_cast<std::size_t>(length), '\0');
            WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()), text.data(), length, nullptr, nullptr);
            return text;
        }

        std::string DistRefsReadEfkefcString(const std::string& chunk, std::size_t& offset)
        {
            const std::int32_t count = DistRefsReadI32(chunk, offset);
            offset += 4;
            if (count < 0 || count > DIST_EFKEFC_MAX_COUNT || offset + static_cast<std::size_t>(count) * 2 > chunk.size())
                throw DistRefsError{};
            std::wstring text = DistRefsUtf16(chunk, offset, static_cast<std::size_t>(count));
            offset += static_cast<std::size_t>(count) * 2;
            while (!text.empty() && text.back() == L'\0')
                text.pop_back();
            return DistRefsWideToUtf8Strict(text);
        }

        std::vector<std::string> DistRefsParseEfkefcInfo(const std::string& chunk)
        {
            if (chunk.size() < 4)
                throw DistRefsError{};
            const std::int32_t version = DistRefsReadI32(chunk, 0);
            std::vector<std::string> paths;
            std::size_t offset = 4;
            if (version <= DIST_EFKEFC_STRING_LISTS_MAX_VERSION)
            {
                while (offset < chunk.size())
                {
                    const std::int32_t count = DistRefsReadI32(chunk, offset);
                    offset += 4;
                    if (count < 0 || count > DIST_EFKEFC_MAX_COUNT)
                        throw DistRefsError{};
                    for (std::int32_t i = 0; i < count; ++i)
                        paths.push_back(DistRefsReadEfkefcString(chunk, offset));
                }
            }
            else
            {
                const std::int32_t count = DistRefsReadI32(chunk, offset);
                offset += 4;
                if (count < 0 || count > DIST_EFKEFC_MAX_COUNT)
                    throw DistRefsError{};
                for (std::int32_t i = 0; i < count; ++i)
                {
                    // NOTE: ファイル種別とフラグを読み飛ばす
                    offset += 8;
                    paths.push_back(DistRefsReadEfkefcString(chunk, offset));
                }
                if (offset != chunk.size())
                    throw DistRefsError{};
            }
            return paths;
        }

        bool DistRefsIsWhitespace(const wchar_t c)
        {
            return (c >= 0x09 && c <= 0x0D) || (c >= 0x1C && c <= 0x20) || c == 0x85 || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A)
                || c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000;
        }

        wchar_t DistRefsAsciiLower(const wchar_t c)
        {
            return (c >= L'A' && c <= L'Z') ? static_cast<wchar_t>(c - L'A' + L'a') : c;
        }

        // NOTE: INFO が見慣れないレイアウトでも、[^\0]+?\.(拡張子) にあたる部分をできる範囲で拾う
        std::vector<std::string> DistRefsScanEfkefcText(const std::string& chunk)
        {
            // NOTE: 奇数バイトの端数と対になっていないサロゲートは捨てる
            if (chunk.size() <= 4)
                return {};
            const std::size_t units = (chunk.size() - 4) / 2;
            const std::wstring raw  = DistRefsUtf16(chunk, 4, units);
            std::wstring text;
            text.reserve(raw.size());
            for (std::size_t i = 0; i < raw.size(); ++i)
            {
                const wchar_t c = raw[i];
                if (c >= 0xD800 && c <= 0xDBFF && i + 1 < raw.size() && raw[i + 1] >= 0xDC00 && raw[i + 1] <= 0xDFFF)
                {
                    text.push_back(c);
                    text.push_back(raw[++i]);
                    continue;
                }
                if (c >= 0xD800 && c <= 0xDFFF)
                    continue;
                text.push_back(c);
            }

            std::vector<std::string> paths;
            std::size_t start = 0;
            while (start < text.size())
            {
                if (text[start] == L'\0')
                {
                    ++start;
                    continue;
                }

                std::size_t matchEnd = 0;
                for (std::size_t dot = start + 1; dot < text.size() && text[dot] != L'\0' && matchEnd == 0; ++dot)
                {
                    if (text[dot] != L'.')
                        continue;
                    for (const std::wstring_view extension : DIST_EFKEFC_ASSET_EXTENSIONS)
                    {
                        if (dot + 1 + extension.size() > text.size())
                            continue;
                        bool same = true;
                        for (std::size_t k = 0; k < extension.size() && same; ++k)
                            same = DistRefsAsciiLower(text[dot + 1 + k]) == extension[k];
                        if (same)
                        {
                            matchEnd = dot + 1 + extension.size();
                            break;
                        }
                    }
                }
                if (matchEnd == 0)
                {
                    // NOTE: この NUL までのどこから始めても一致しない
                    while (start < text.size() && text[start] != L'\0')
                        ++start;
                    continue;
                }

                std::size_t first = start;
                std::size_t last  = matchEnd;
                while (first < last && DistRefsIsWhitespace(text[first]))
                    ++first;
                while (last > first && DistRefsIsWhitespace(text[last - 1]))
                    --last;
                paths.push_back(WideToUtf8(text.substr(first, last - first)));
                start = matchEnd;
            }
            return paths;
        }

        bool DistRefsIsMv1StringByte(const unsigned char c)
        {
            return (c >= 0x20 && c <= 0x7E) || (c >= 0x80 && c <= 0xFC);
        }

        std::optional<std::string> DistRefsReadFile(const std::filesystem::path& path)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream)
                return std::nullopt;
            std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            if (stream.bad())
                return std::nullopt;
            return bytes;
        }

        // NOTE: 小文字にして区切りを '\' にそろえる (Windows 流のパス正規化)
        std::wstring DistRefsNormCase(std::wstring text)
        {
            std::ranges::replace(text, L'/', L'\\');
            CharLowerBuffW(text.data(), static_cast<DWORD>(text.size()));
            return text;
        }

        // NOTE: child が parent の下にあれば '/' 区切りの相対パス (大文字小文字は区別しない)
        std::optional<std::string> DistRefsRelativeTo(const std::filesystem::path& child, const std::filesystem::path& parent)
        {
            const std::wstring childText  = child.wstring();
            std::wstring       parentText = DistRefsNormCase(parent.wstring());
            while (!parentText.empty() && parentText.back() == L'\\')
                parentText.pop_back();
            parentText.push_back(L'\\');

            if (!DistRefsNormCase(childText).starts_with(parentText))
                return std::nullopt;
            std::wstring rest = childText.substr(parentText.size());
            std::ranges::replace(rest, L'\\', L'/');
            return WideToUtf8(rest);
        }

        std::filesystem::path DistRefsResolve(const std::filesystem::path& owner, std::string ref)
        {
            std::ranges::replace(ref, '\\', '/');
            std::filesystem::path target = Utf8ToPath(ref);
            if (!target.is_absolute())
                target = owner.parent_path() / target;
            return target.lexically_normal();
        }
    }

    std::optional<std::vector<std::string>> EfkefcAssetPaths(const std::string& bytes)
    {
        try
        {
            const std::optional<std::string> info = DistRefsEfkefcChunk(bytes, "INFO");
            if (!info)
                return std::nullopt;
            try
            {
                return DistRefsParseEfkefcInfo(*info);
            }
            catch (const DistRefsError&)
            {
                return DistRefsScanEfkefcText(*info);
            }
        }
        catch (const DistRefsError&)
        {
            return std::nullopt;
        }
    }

    std::optional<std::string> Mv1Decode(const std::string& bytes)
    {
        if (bytes.size() < 4 || bytes.compare(0, 4, DIST_MV1_MAGIC) != 0)
            return std::nullopt;
        const std::string_view body(bytes.data() + 4, bytes.size() - 4);
        if (body.size() < DIST_MV1_HEADER_BYTES)
            return std::nullopt;

        std::uint32_t destinationSize = 0;
        std::uint32_t sourceSize      = 0;
        std::memcpy(&destinationSize, body.data(), 4);
        std::memcpy(&sourceSize,      body.data() + 4, 4);
        const unsigned char key = static_cast<unsigned char>(body[8]);
        if (sourceSize > body.size())
            return std::nullopt;

        const auto at = [&body](const std::size_t index) -> unsigned char
        {
            if (index >= body.size())
                throw DistRefsError{};
            return static_cast<unsigned char>(body[index]);
        };

        std::string out;
        // NOTE: 壊れたヘッダの巨大なサイズで確保しない
        out.reserve((std::min)(static_cast<std::size_t>(destinationSize), DIST_MV1_MAX_RESERVE_BYTES));
        try
        {
            std::size_t sp = DIST_MV1_HEADER_BYTES;
            while (sp < sourceSize)
            {
                const unsigned char c = at(sp);
                if (c != key)
                {
                    out.push_back(static_cast<char>(c));
                    ++sp;
                    continue;
                }
                if (at(sp + 1) == key)
                {
                    out.push_back(static_cast<char>(key));
                    sp += 2;
                    continue;
                }
                unsigned int code = at(sp + 1);
                if (code > key)
                    --code;
                sp += 2;
                std::size_t length = code >> 3;
                if (code & 0x4)
                {
                    length |= static_cast<std::size_t>(at(sp)) << 5;
                    ++sp;
                }
                length += DIST_MV1_MIN_MATCH;

                std::size_t index = 0;
                switch (code & 0x3)
                {
                case 0:
                    index = at(sp);
                    sp += 1;
                    break;
                case 1:
                    index = at(sp) | static_cast<std::size_t>(at(sp + 1)) << 8;
                    sp += 2;
                    break;
                default:
                    index = at(sp) | static_cast<std::size_t>(at(sp + 1)) << 8 | static_cast<std::size_t>(at(sp + 2)) << 16;
                    sp += 3;
                    break;
                }
                if (index + 1 > out.size())
                    return std::nullopt;
                const std::size_t start = out.size() - (index + 1);
                // NOTE: 重なる一致 (距離 < 長さ) があるので 1 バイトずつ写す
                for (std::size_t i = 0; i < length; ++i)
                    out.push_back(out[start + i]);
                if (out.size() > destinationSize)
                    return std::nullopt;
            }
        }
        catch (const DistRefsError&)
        {
            return std::nullopt;
        }

        if (out.size() != destinationSize)
            return std::nullopt;
        return out;
    }

    std::optional<std::vector<std::string>> Mv1TexturePaths(const std::string& bytes)
    {
        const std::optional<std::string> decoded = Mv1Decode(bytes);
        if (!decoded)
            return std::nullopt;
        const std::string& body = *decoded;

        std::vector<std::string>        found;
        std::unordered_set<std::string> seen;
        std::size_t i = 0;
        while (i < body.size())
        {
            if (!DistRefsIsMv1StringByte(static_cast<unsigned char>(body[i])))
            {
                ++i;
                continue;
            }
            const std::size_t runStart = i;
            std::size_t end = i;
            while (end < body.size() && DistRefsIsMv1StringByte(static_cast<unsigned char>(body[end])))
                ++end;
            i = end;
            if (end >= body.size() || body[end] != '\0')
                continue;

            // NOTE: [\x20-\x7e\x80-\xfc]{1,259}\.(拡張子)(?=\x00)。一致は NUL の直前で終わり、先頭は最大 259 バイト前まで
            for (const std::string_view extension : DIST_MV1_TEXTURE_EXTENSIONS)
            {
                if (end < runStart + extension.size() + 2)
                    continue;
                const std::size_t dot = end - extension.size() - 1;
                if (body[dot] != '.' || AsciiLower(body.substr(dot + 1, extension.size())) != extension)
                    continue;
                const std::size_t first = (std::max)(runStart, dot > DIST_MV1_MAX_PREFIX ? dot - DIST_MV1_MAX_PREFIX : 0);
                std::string text = DecodeUtf8OrCp932(body.substr(first, end - first));
                if (seen.insert(text).second)
                    found.push_back(std::move(text));
                break;
            }
        }
        return found;
    }

    std::optional<std::vector<std::string>> ReferencesOf(const std::filesystem::path& path)
    {
        const std::optional<std::string> bytes = DistRefsReadFile(path);
        if (!bytes)
            return std::nullopt;
        std::wstring extension = path.extension().wstring();
        CharLowerBuffW(extension.data(), static_cast<DWORD>(extension.size()));
        if (extension == L".efkefc")
            return EfkefcAssetPaths(*bytes);
        return Mv1TexturePaths(*bytes);
    }

    DistRefReport FindUnshippedRefs(const DistScanResult& scan, const std::filesystem::path& repoRoot, const std::filesystem::path& assetsRoot,
                                    DistHashCache& cache, const std::function<bool()>& isCanceled)
    {
        const std::filesystem::path repo   = std::filesystem::absolute(repoRoot).lexically_normal();
        const std::filesystem::path assets = std::filesystem::absolute(assetsRoot).lexically_normal();

        DistRefReport report;
        for (const ManifestEntry& entry : scan.entries)
        {
            const std::string lowered = AsciiLower(entry.path);
            if (std::ranges::none_of(DIST_REFERRING_SUFFIXES, [&lowered](const std::string_view suffix) { return lowered.ends_with(suffix); }))
                continue;
            if (isCanceled && isCanceled())
            {
                report.canceled = true;
                return report;
            }

            const std::filesystem::path owner = repo / Utf8ToPath(entry.path);
            const DistHashCache::References refs = cache.RefsOf(entry.hash, [&owner] { return ReferencesOf(owner); });
            if (!refs)
            {
                report.unreadable.push_back(entry.path);
                continue;
            }

            std::unordered_set<std::string> seen;
            for (const std::string& ref : *refs)
            {
                if (ref.empty() || !seen.insert(ref).second)
                    continue;
                const std::filesystem::path target = DistRefsResolve(owner, ref);
                std::error_code error;
                if (!std::filesystem::is_regular_file(target, error))
                    continue;
                if (!DistRefsRelativeTo(target, assets))
                {
                    report.unshipped.push_back({ entry.path, ref, DIST_REASON_OUTSIDE });
                    continue;
                }
                const std::optional<std::string> rel = DistRefsRelativeTo(target, repo);
                if (!rel || IsExcludedFromDistribution(*rel))
                    report.unshipped.push_back({ entry.path, ref, DIST_REASON_EXCLUDED });
            }
        }
        return report;
    }
}
