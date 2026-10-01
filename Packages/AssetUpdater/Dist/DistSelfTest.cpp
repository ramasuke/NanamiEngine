#include "DistSelfTest.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <windows.h>

#include "DistConfig.h"
#include "DistExclusion.h"
#include "DistHashCache.h"
#include "DistManifestBuilder.h"
#include "DistReferences.h"
#include "DistScan.h"
#include "DistUpload.h"
#include "Rclone.h"
#include "../Hash/Sha256.h"
#include "../Text/Utf8.h"
#include "../Text/VersionString.h"
#include "../cereal/include/cereal/external/rapidjson/document.h"
#include "../cereal/include/cereal/external/rapidjson/stringbuffer.h"
#include "../cereal/include/cereal/external/rapidjson/writer.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        struct DistSelfTestFailure
        {
            std::string message;
        };

        void DistSelfTestExpect(const bool condition, const std::string& message)
        {
            if (!condition)
                throw DistSelfTestFailure{ message };
        }

        class DistSelfTestReporter final
        {
        public:
            explicit DistSelfTestReporter(const std::function<void(std::string)>& print) : print_(print) {}

            void Ok     (const std::string& name) { ++passed_; print_("  PASS  " + name); }
            void Skip   (const std::string& name, const std::string& why) { print_("  SKIP  " + name + " (" + why + ")"); }
            void Section(const std::string& title) { print_(""); print_("=== " + title + " ==="); }
            void Fail   (const std::string& name, const std::string& error)
            {
                ++failed_;
                print_("  FAIL  " + name);
                print_("        " + error);
            }

            /** body の中の Expect が外れたら name を FAIL にする */
            template <class Body>
            void Run(const std::string& name, Body&& body)
            {
                try
                {
                    body();
                }
                catch (const DistSelfTestFailure& failure)
                {
                    Fail(name, failure.message);
                }
                catch (const std::exception& exception)
                {
                    Fail(name, std::string("exception: ") + exception.what());
                }
            }

            int Finish() const
            {
                print_("");
                print_(std::format("{}/{} checks passed{}", passed_, passed_ + failed_, failed_ ? std::format(", {} FAILED", failed_) : std::string()));
                return failed_ ? 1 : 0;
            }

        private:
            const std::function<void(std::string)>& print_;
            int passed_ = 0;
            int failed_ = 0;
        };

        class DistSelfTestTempDir final
        {
        public:
            DistSelfTestTempDir()
            {
                static int counter = 0;
                const std::filesystem::path base = std::filesystem::temp_directory_path();
                path_ = base / std::format(L"nanami-dist-selftest-{}-{}-{}", GetCurrentProcessId(), GetTickCount64(), ++counter);
                std::filesystem::create_directories(path_);
                path_ = std::filesystem::absolute(path_).lexically_normal();
            }
            ~DistSelfTestTempDir()
            {
                std::error_code error;
                std::filesystem::remove_all(path_, error);
            }
            DistSelfTestTempDir(const DistSelfTestTempDir&)            = delete;
            DistSelfTestTempDir& operator=(const DistSelfTestTempDir&) = delete;

            [[nodiscard]] const std::filesystem::path& Path() const { return path_; }

        private:
            std::filesystem::path path_;
        };

        void DistSelfTestWrite(const std::filesystem::path& root, const std::string& rel, const std::string& bytes)
        {
            const std::filesystem::path path = root / Utf8ToPath(rel);
            std::filesystem::create_directories(path.parent_path());
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }

        std::string DistSelfTestRead(const std::filesystem::path& path)
        {
            return ReadLocalFile(path);
        }

        std::string DistSelfTestMetaJson(const std::string& rel, const std::string& guid)
        {
            std::string escaped;
            for (const char c : rel)
                escaped += c == '/' ? std::string("\\\\") : std::string(1, c);
            return "{\n    \"value0\": {\n        \"polymorphic_id\": 2147483649,\n        \"polymorphic_name\": \"NanamiEngine::Module::Asset::Mv1File\",\n"
                   "        \"ptr_wrapper\": {\n            \"id\": 2147483649,\n            \"data\": {\n                \"cereal_class_version\": 1,\n"
                   "                \"value0\": {\"cereal_class_version\": 0},\n                \"contentPath_\": \"" + escaped + "\",\n"
                   "                \"guid_\": {\"cereal_class_version\": 0, \"value_\": \"" + guid + "\"}\n            }\n        }\n    }\n}";
        }

        /** Assets/ の実際の形を小さく再現する */
        void DistSelfTestMakeTree(const std::filesystem::path& root)
        {
            // アセット: 本体 + .meta
            DistSelfTestWrite(root, "Assets/Art/Models/Hyena.mv1", "hyena-model");
            DistSelfTestWrite(root, "Assets/Art/Models/Hyena.mv1.meta", DistSelfTestMetaJson("Assets/Art/Models/Hyena.mv1", "11111111-1111-1111-1111-111111111111"));
            DistSelfTestWrite(root, "Assets/Audio/Howl.mp3", "howl-sound");
            DistSelfTestWrite(root, "Assets/Audio/Howl.mp3.meta", DistSelfTestMetaJson("Assets/Audio/Howl.mp3", "22222222-2222-2222-2222-222222222222"));

            // 随伴ファイル: .meta 無しだが実行時に要る
            DistSelfTestWrite(root, "Assets/Art/Models/Hyena.fbm/Hyena_Diffuse.png", "hyena-texture");
            DistSelfTestWrite(root, "Assets/Art/Effect/Foo/Model/rock1.efkmodel", "efk-model");
            DistSelfTestWrite(root, "Assets/Art/Shaders/Tree/Tree_VS.vso", "compiled-shader");

            // 開発専用: 落ちるべきもの
            DistSelfTestWrite(root, "Assets/Art/Models/Hyena.fbx", "source-model");
            DistSelfTestWrite(root, "Assets/Art/Effect/_Source/Foo/Foo.efkproj", "source-effect");
            DistSelfTestWrite(root, "Assets/Art/Effect/_Source/Foo/Model/rock1.efkmodel", "source-companion");
            DistSelfTestWrite(root, "Assets/Scripts/Core/Game/Game.cpp", "code");
            DistSelfTestWrite(root, "Assets/Scripts/Core/Game/Game.h", "code");
            DistSelfTestWrite(root, "Assets/Art/Models/desktop.ini", "junk");
            DistSelfTestWrite(root, "Assets/Data/Thing.swordManResource.meta.bak", "backup");
        }

        void DistSelfTestAppendI32(std::string& out, const std::int32_t value)
        {
            char bytes[4];
            std::memcpy(bytes, &value, 4);
            out.append(bytes, 4);
        }

        /** EFKE ヘッダ + 1710 形式の INFO (dependency list) だけの .efkefc */
        std::string DistSelfTestEfkefc(const std::vector<std::string>& paths)
        {
            std::string info;
            DistSelfTestAppendI32(info, 1710);
            DistSelfTestAppendI32(info, static_cast<std::int32_t>(paths.size()));
            for (const std::string& path : paths)
            {
                const std::wstring wide = Utf8ToWide(path) + L'\0';
                DistSelfTestAppendI32(info, 0);
                DistSelfTestAppendI32(info, 0);
                DistSelfTestAppendI32(info, static_cast<std::int32_t>(wide.size()));
                info.append(reinterpret_cast<const char*>(wide.data()), wide.size() * sizeof(wchar_t));
            }
            std::string file = "EFKE";
            DistSelfTestAppendI32(file, 0);
            file += "INFO";
            DistSelfTestAppendI32(file, static_cast<std::int32_t>(info.size()));
            return file + info;
        }

        /** 圧縮なし (キーバイト 0xFF が本文に現れない) の LZ ストリームを持つ .mv1 */
        std::string DistSelfTestMv1(const std::vector<std::string>& paths)
        {
            std::string body("MV1-body\0", 9);
            for (const std::string& path : paths)
            {
                body += path;
                body.push_back('\0');
            }
            std::string file = "MV11";
            DistSelfTestAppendI32(file, static_cast<std::int32_t>(body.size()));
            DistSelfTestAppendI32(file, static_cast<std::int32_t>(9 + body.size()));
            file.push_back(static_cast<char>(0xFF));
            return file + body;
        }

        void DistSelfTestMakeRefTree(const std::filesystem::path& root)
        {
            DistSelfTestWrite(root, "Outside/tex.png", "desktop-only");
            DistSelfTestWrite(root, "Assets/Art/Effect/Foo/Texture/ok.png", "shipped");
            DistSelfTestWrite(root, "Assets/Art/Effect/_Source/Foo/src.png", "source-only");
            DistSelfTestWrite(root, "Assets/Art/Effect/Foo/Foo.efkefc", DistSelfTestEfkefc({
                "Texture/ok.png",
                "../../../../Outside/tex.png",
                "../_Source/Foo/src.png",
                "Texture/missing.png",
            }));
            DistSelfTestWrite(root, "Assets/Art/Models/textures/ok.png", "shipped");
            DistSelfTestWrite(root, "Assets/Art/Models/M.mv1", DistSelfTestMv1({
                "textures\\ok.png",
                "..\\..\\..\\Outside\\tex.png",
                "C:\\NoSuchSourceDir\\ok.png",
            }));
            DistSelfTestWrite(root, "Assets/Art/Models/Broken.mv1", "hyena-model");
        }

        const ManifestEntry& DistSelfTestFind(const std::vector<ManifestEntry>& entries, const std::string& suffix)
        {
            const auto found = std::ranges::find_if(entries, [&suffix](const ManifestEntry& e) { return e.path.ends_with(suffix); });
            DistSelfTestExpect(found != entries.end(), "entry が無い: " + suffix);
            return *found;
        }

        ManifestEntry DistSelfTestEntry(const std::string& path, const std::string& hash, const std::string& metaHash = {}, const std::uint64_t size = 100, const std::uint64_t metaSize = 0)
        {
            ManifestEntry entry;
            entry.path     = path;
            entry.hash     = hash;
            entry.metaHash = metaHash;
            entry.size     = size;
            entry.metaSize = metaSize;
            return entry;
        }

        AssetManifest DistSelfTestManifest(std::vector<ManifestEntry> entries)
        {
            AssetManifest manifest;
            manifest.version = "x";
            manifest.entries = std::move(entries);
            return manifest;
        }

        std::vector<std::string> DistSelfTestPaths(const std::vector<ManifestEntry>& entries)
        {
            std::vector<std::string> paths;
            for (const ManifestEntry& entry : entries)
                paths.push_back(entry.path);
            std::ranges::sort(paths);
            return paths;
        }

        std::string DistSelfTestJoin(const std::vector<std::string>& items)
        {
            std::string text;
            for (const std::string& item : items)
                text += (text.empty() ? "" : ", ") + item;
            return "[" + text + "]";
        }

        // Packages/AssetUpdater/Manifest/AssetManifest.cpp が読むキーそのもの
        void StageJsonShape(DistSelfTestReporter& r)
        {
            r.Section("stage 0: JSON shape matches the C++ reader");
            r.Run("json shape", [&]
            {
                DistScanResult scan;
                ManifestEntry entry = DistSelfTestEntry("Assets/a.mv1", "deadbeef", "cafe", 10, 2);
                entry.guid = "G";
                scan.entries.push_back(entry);
                const AssetManifest manifest = BuildManifest(scan, "1.1.0", "1.0.0", "https://host/1.1.0/");
                const std::string   json     = SerializeManifest(manifest);

                CEREAL_RAPIDJSON_NAMESPACE::Document document;
                document.Parse(json.c_str(), json.size());
                DistSelfTestExpect(!document.HasParseError() && document.IsObject(), "JSON として読めない");
                std::set<std::string> entryKeys;
                const auto& first = document["entries"][0u];
                for (auto it = first.MemberBegin(); it != first.MemberEnd(); ++it)
                    entryKeys.insert(it->name.GetString());
                DistSelfTestExpect(entryKeys == std::set<std::string>{ "guid", "path", "hash", "size", "metaHash", "metaSize" }, "entry のキーが違う");
                r.Ok("entry keys are exactly guid/path/hash/size/metaHash/metaSize");

                for (const char* key : { "schema", "version", "requiredClientVersion", "baseUrl", "entries" })
                    DistSelfTestExpect(document.HasMember(key), std::string("root に無い: ") + key);
                DistSelfTestExpect(document["schema"].GetInt() == DIST_MANIFEST_SCHEMA, "schema");
                DistSelfTestExpect(first["metaSize"].GetUint64() == 2, "metaSize");
                r.Ok("root keys include schema/version/requiredClientVersion/baseUrl/entries");

                const DistSelfTestTempDir tmp;
                const std::filesystem::path out = tmp.Path() / L"manifest.json";
                DistSelfTestExpect(WriteManifestFile(manifest, out).empty(), "書けない");
                const std::string raw = DistSelfTestRead(out);
                DistSelfTestExpect(!raw.starts_with("\xEF\xBB\xBF"), "BOM があると rapidjson が困る");
                DistSelfTestExpect(raw.find("\r\n") == std::string::npos, "CRLF ではなく LF で書く");
                AssetManifest loaded;
                std::string   error;
                DistSelfTestExpect(AssetManifest::TryLoadFile(out, loaded, error), "読み戻せない: " + error);
                DistSelfTestExpect(loaded.version == "1.1.0" && loaded.requiredClientVersion == "1.0.0" && loaded.baseUrl == "https://host/1.1.0/"
                                   && loaded.entries.size() == 1 && loaded.entries[0].guid == "G" && loaded.entries[0].metaHash == "cafe", "round-trip で値が変わる");
                r.Ok("dump is UTF-8 / LF / no BOM and round-trips");
            });
        }

        void StageExclusions(DistSelfTestReporter& r)
        {
            r.Section("stage 1: exclusion rules");
            const std::vector<std::string> shipped =
            {
                "Assets/Art/Models/Hyena.mv1",
                "Assets/Art/Models/Hyena.fbm/Hyena_Diffuse.png",
                "Assets/Art/Effect/Foo/Model/rock1.efkmodel",
                "Assets/Art/Shaders/Tree/Tree_VS.vso",
                "Assets/Art/Shaders/Tree/Tree_PS.pso",
                "Assets/Art/Models/Dragon/M_Body.mat",
                // 同名の原本が _Source/ にあっても、.mv1 が参照するのは隣のこちら
                "Assets/Art/Models/Prop/EventBoard/EventNoticeBoard_Wood.png",
                // 名前に "source" を含むだけのディレクトリは対象外
                "Assets/Art/UI/SourceFrame/Frame.png",
            };
            const std::vector<std::string> dropped =
            {
                "Assets/Art/Models/Hyena.mv1.meta",
                "Assets/Art/Models/Hyena.fbx",
                "Assets/Art/Effect/_Source/Foo/Foo.efkproj",
                "Assets/Art/Effect/_Source/Foo/Model/rock1.efkmodel",
                "Assets/Art/Models/Nature/Trees/_Source/Trees.blend",
                "Assets/Art/Models/Prop/EventBoard/_Source/EventNoticeBoard_Wood.png",
                "Assets/Art/UI/StageSelect/_Source/SelectFrameGlow_Source.png",
                "Assets/Art/Models/Somewhere/Model.blend",
                "Assets/Art/Models/Somewhere/Model.blend1",
                "Assets/Scripts/Core/Game/Game.cpp",
                "Assets/Scripts/Core/Game/Game.h",
                "Assets/Art/Models/desktop.ini",
                "Assets/Data/Thing.swordManResource.meta.bak",
            };
            r.Run("exclusions", [&]
            {
                for (const std::string& path : shipped)
                    DistSelfTestExpect(!IsExcludedFromDistribution(path), "配信すべきなのに除外された: " + path);
                r.Ok(std::format("{} runtime files are kept (companions included)", shipped.size()));
                for (const std::string& path : dropped)
                    DistSelfTestExpect(IsExcludedFromDistribution(path), "除外すべきなのに残った: " + path);
                r.Ok(std::format("{} dev-only files are dropped", dropped.size()));
            });
        }

        void StageRealGuid(DistSelfTestReporter& r, const std::filesystem::path& repoRoot)
        {
            r.Section("stage 2: guid read from committed .meta fixtures");
            const std::filesystem::path thin = repoRoot / L"Assets/Brute.mv1.meta";
            const std::filesystem::path fat  = repoRoot / Utf8ToPath("Assets/Data/PlayerAvatar/InitStatus/SwordMan/SwordManInitStatus.swordManInitStatus.meta");

            std::error_code error;
            if (std::filesystem::exists(thin, error))
            {
                r.Run("thin proxy guid", [&]
                {
                    const DistMetaGuid guid = ReadMetaGuid(thin);
                    DistSelfTestExpect(guid.guid == "6231DCB3-3256-41FC-9276-3998037AA2A1", guid.guid);
                    DistSelfTestExpect(guid.encoding == "utf-8", guid.encoding);
                    r.Ok("thin proxy (Mv1File) guid");
                });
            }
            else
            {
                r.Skip("thin proxy guid", "Assets/Brute.mv1.meta が無い");
            }

            if (std::filesystem::exists(fat, error))
            {
                r.Run("fat ScriptableObject guid", [&]
                {
                    const DistMetaGuid guid = ReadMetaGuid(fat);
                    DistSelfTestExpect(guid.guid.size() == 36 && std::ranges::count(guid.guid, '-') == 4, guid.guid);
                    r.Ok("fat ScriptableObject (SwordManInitStatus) guid");
                });
            }
            else
            {
                r.Skip("fat ScriptableObject guid", "fixture が無い");
            }

            r.Run("CP932 .meta", [&]
            {
                // UTF-8 化以前のエンジンが書いた .meta は CP932 のまま残っている。contentPath_ の日本語が UTF-8 として不正
                const std::wstring body = L"{\"value0\":{\"ptr_wrapper\":{\"data\":{\"contentPath_\":\"Assets\\\\Audio\\\\ドラゴンの鳴き声1.mp3\","
                                          L"\"guid_\":{\"value_\":\"AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE\"}}}}}";
                const int length = WideCharToMultiByte(932, 0, body.data(), static_cast<int>(body.size()), nullptr, 0, nullptr, nullptr);
                std::string cp932(static_cast<size_t>(length), '\0');
                WideCharToMultiByte(932, 0, body.data(), static_cast<int>(body.size()), cp932.data(), length, nullptr, nullptr);

                const DistSelfTestTempDir tmp;
                DistSelfTestWrite(tmp.Path(), "sjis.meta", cp932);
                const DistMetaGuid guid = ReadMetaGuid(tmp.Path() / L"sjis.meta");
                DistSelfTestExpect(guid.guid == "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", guid.guid);
                DistSelfTestExpect(guid.encoding == "cp932", guid.encoding);
                r.Ok("CP932 .meta still yields its guid");
            });

            r.Run("broken .meta", [&]
            {
                const DistSelfTestTempDir tmp;
                DistSelfTestWrite(tmp.Path(), "broken.meta", "{ this is not json");
                const DistMetaGuid guid = ReadMetaGuid(tmp.Path() / L"broken.meta");
                DistSelfTestExpect(guid.guid.empty() && guid.encoding.empty(), guid.guid + " / " + guid.encoding);
                r.Ok("a broken .meta yields '' instead of failing the build");
            });
        }

        void StageScan(DistSelfTestReporter& r)
        {
            r.Section("stage 3: scan splits assets from companions");
            r.Run("scan", [&]
            {
                const DistSelfTestTempDir tmp;
                const std::filesystem::path& root = tmp.Path();
                DistSelfTestMakeTree(root);
                DistHashCache cache({});
                const DistScanResult result = ScanAssets(root / L"Assets", root, cache);
                DistSelfTestExpect(result.error.empty(), result.error);

                const std::vector<std::string> paths = DistSelfTestPaths(result.entries);
                const std::vector<std::string> expected =
                {
                    "Assets/Art/Effect/Foo/Model/rock1.efkmodel",
                    "Assets/Art/Models/Hyena.fbm/Hyena_Diffuse.png",
                    "Assets/Art/Models/Hyena.mv1",
                    "Assets/Art/Shaders/Tree/Tree_VS.vso",
                    "Assets/Audio/Howl.mp3",
                };
                DistSelfTestExpect(paths == expected, DistSelfTestJoin(paths));
                r.Ok("only runtime files become entries");

                DistSelfTestExpect(result.AssetCount() == 2 && result.CompanionCount() == 3, std::format("{} / {}", result.AssetCount(), result.CompanionCount()));
                r.Ok("2 assets (with .meta) / 3 companions (without)");

                const ManifestEntry& hyena = DistSelfTestFind(result.entries, "Hyena.mv1");
                DistSelfTestExpect(hyena.guid == "11111111-1111-1111-1111-111111111111", hyena.guid);
                DistSelfTestExpect(hyena.size == std::string("hyena-model").size(), "size");
                DistSelfTestExpect(hyena.metaSize > 0 && !hyena.metaHash.empty(), "meta");
                r.Ok("asset entry carries guid + metaHash + metaSize");

                const ManifestEntry& texture = DistSelfTestFind(result.entries, "Hyena_Diffuse.png");
                DistSelfTestExpect(texture.guid.empty() && texture.metaHash.empty() && texture.metaSize == 0 && texture.TotalSize() == texture.size, "companion");
                r.Ok("companion entry has no guid/meta and is identified by path");

                DistSelfTestExpect(result.missingGuid.empty(), DistSelfTestJoin(result.missingGuid));
                r.Ok("no .meta failed to yield a guid");
            });
        }

        void StageDiff(DistSelfTestReporter& r)
        {
            r.Section("stage 4: diff (ManifestDiff::Between)");
            r.Run("diff", [&]
            {
                const AssetManifest installed = DistSelfTestManifest({
                    DistSelfTestEntry("same.mv1", "aaa", "m1"),
                    DistSelfTestEntry("body-changed.mv1", "bbb", "m2"),
                    DistSelfTestEntry("meta-changed.mv1", "ccc", "m3"),
                    DistSelfTestEntry("gone.mv1", "ddd", "m4"),
                });
                const AssetManifest remote = DistSelfTestManifest({
                    DistSelfTestEntry("same.mv1", "aaa", "m1"),
                    DistSelfTestEntry("body-changed.mv1", "BBB", "m2"),
                    DistSelfTestEntry("meta-changed.mv1", "ccc", "M3"),
                    DistSelfTestEntry("new.mv1", "eee", "m5", 50, 7),
                });

                const ManifestDiff diff = ManifestDiff::Between(installed, remote);
                DistSelfTestExpect(DistSelfTestPaths(diff.added) == std::vector<std::string>{ "new.mv1" }, DistSelfTestJoin(DistSelfTestPaths(diff.added)));
                DistSelfTestExpect(DistSelfTestPaths(diff.changed) == std::vector<std::string>{ "body-changed.mv1", "meta-changed.mv1" }, DistSelfTestJoin(DistSelfTestPaths(diff.changed)));
                DistSelfTestExpect(diff.removedPaths == std::vector<std::string>{ "gone.mv1" }, DistSelfTestJoin(diff.removedPaths));
                r.Ok("added / changed (body or meta) / removed");

                DistSelfTestExpect(diff.downloadBytes == 100 + 100 + 57, std::to_string(diff.downloadBytes));
                r.Ok("download bytes count body + meta of added and changed only");

                DistSelfTestExpect(!diff.IsUpToDate() && ManifestDiff::Between(installed, installed).IsUpToDate(), "up to date");
                r.Ok("identical manifests are up to date");

                // installed.json が無いときは全件が新規
                const ManifestDiff fresh = ManifestDiff::Between(DistSelfTestManifest({}), remote);
                DistSelfTestExpect(fresh.added.size() == 4 && fresh.changed.empty() && fresh.removedPaths.empty(), "fresh");
                r.Ok("empty installed state makes every entry 'added'");
            });
        }

        void StageHashCache(DistSelfTestReporter& r)
        {
            r.Section("stage 5: hash cache");
            r.Run("hash cache", [&]
            {
                const DistSelfTestTempDir tmp;
                const std::filesystem::path& root = tmp.Path();
                DistSelfTestMakeTree(root);
                const std::filesystem::path cachePath = root / L"cache.json";

                DistHashCache first(cachePath);
                const DistScanResult before = ScanAssets(root / L"Assets", root, first);
                first.Save();
                DistSelfTestExpect(first.misses > 0 && first.hits == 0, std::format("misses {} hits {}", first.misses, first.hits));
                r.Ok(std::format("cold run hashes everything ({} files)", first.misses));

                DistHashCache second(cachePath);
                const DistScanResult after = ScanAssets(root / L"Assets", root, second);
                DistSelfTestExpect(second.misses == 0, std::format("{} files were re-hashed", second.misses));
                DistSelfTestExpect(second.hits == first.misses, std::format("hits {}", second.hits));
                r.Ok("warm run re-hashes nothing");

                DistSelfTestExpect(before.entries.size() == after.entries.size(), "entries");
                for (size_t i = 0; i < before.entries.size(); ++i)
                    DistSelfTestExpect(before.entries[i].hash == after.entries[i].hash, "hash: " + before.entries[i].path);
                r.Ok("cached hashes equal freshly computed ones");

                DistSelfTestWrite(root, "Assets/Art/Models/Hyena.mv1", "hyena-model-v2");
                DistHashCache third(cachePath);
                const DistScanResult result = ScanAssets(root / L"Assets", root, third);
                DistSelfTestExpect(DistSelfTestFind(result.entries, "Hyena.mv1").hash != DistSelfTestFind(before.entries, "Hyena.mv1").hash, "hash unchanged");
                DistSelfTestExpect(third.misses >= 1, "not re-hashed");
                r.Ok("a rewritten file invalidates its cache entry");
            });
        }

        void StageUpload(DistSelfTestReporter& r)
        {
            r.Section("stage 6: upload plan and staging");
            r.Run("upload plan", [&]
            {
                const std::filesystem::path root = L"R";
                const AssetManifest document = DistSelfTestManifest({
                    DistSelfTestEntry("Assets/a.mv1", "h1", "m1", 10, 2),
                    DistSelfTestEntry("Assets/b.mv1", "h1", "m2", 10, 3),
                    DistSelfTestEntry("Assets/c.png", "h3", "", 5, 0),
                });
                const std::map<std::string, DistBlob> blobs = BlobsOf(document, root);
                std::vector<std::string> keys;
                for (const auto& [digest, blob] : blobs)
                    keys.push_back(digest);
                DistSelfTestExpect(keys == std::vector<std::string>{ "h1", "h3", "m1", "m2" }, DistSelfTestJoin(keys));
                r.Ok("body and .meta are separate blobs; identical content collapses; companions have no meta blob");

                DistSelfTestExpect(blobs.at("m1").source == root / Utf8ToPath("Assets/a.mv1.meta"), "meta source");
                r.Ok(".meta blob is read from <path>.meta");

                const std::vector<DistBlob> todo = PlanUpload(blobs, { "h1", "m2" });
                std::vector<std::string> planned;
                for (const DistBlob& blob : todo)
                    planned.push_back(blob.hash);
                DistSelfTestExpect(planned == std::vector<std::string>{ "h3", "m1" }, DistSelfTestJoin(planned));
                r.Ok("only hashes missing on the remote are planned");
            });

            r.Run("upload staging", [&]
            {
                const DistSelfTestTempDir tmp;
                const std::filesystem::path& root = tmp.Path();
                DistSelfTestMakeTree(root);
                DistHashCache cache({});
                const DistScanResult result   = ScanAssets(root / L"Assets", root, cache);
                const AssetManifest  document = BuildManifest(result, "1.0.0", "1.0.0", "https://x/files/");
                const std::map<std::string, DistBlob> blobs = BlobsOf(document, root);
                const std::vector<DistBlob> todo = PlanUpload(blobs, {});

                const std::filesystem::path staging = root / L"staging";
                std::filesystem::create_directories(staging);
                DistSelfTestExpect(StageBlobs(todo, staging).empty(), "stage に問題が出た");
                size_t staged = 0;
                for (const auto& file : std::filesystem::directory_iterator(staging))
                {
                    ++staged;
                    const std::string name = WideToUtf8(file.path().filename().wstring());
                    DistSelfTestExpect(blobs.contains(name), "余計なブロブ: " + name);
                    DistSelfTestExpect(Sha256OfFile(file.path()) == name, "名前とハッシュが違う: " + name);
                }
                DistSelfTestExpect(staged == blobs.size(), std::format("{} / {}", staged, blobs.size()));
                r.Ok(std::format("every staged blob is named by its own SHA-256 ({} blobs)", staged));

                DistSelfTestWrite(root, "Assets/Audio/Howl.mp3", "edited after build");
                const std::string howlHash = DistSelfTestFind(result.entries, "Howl.mp3").hash;
                const std::filesystem::path stagingAgain = root / L"staging_again";
                std::filesystem::create_directories(stagingAgain);
                const std::vector<std::string> problems = StageBlobs(todo, stagingAgain);
                DistSelfTestExpect(problems.size() == 1 && problems[0].find("Howl.mp3") != std::string::npos, DistSelfTestJoin(problems));
                DistSelfTestExpect(!std::filesystem::exists(stagingAgain / Utf8ToPath(howlHash)), "食い違ったブロブが残った");
                r.Ok("a file edited after build is reported and never staged");

                DistSelfTestExpect(StageBlobs(todo, {}).size() == 1, "dry-run");
                r.Ok("dry-run verify reports the same mismatch without copying");
            });

            r.Run("config urls", [&]
            {
                const DistConfig withSlash{ "r2:b", "https://pub-x.r2.dev/", "rclone" };
                const DistConfig withoutSlash{ "r2:b", "https://pub-x.r2.dev", "rclone" };
                DistSelfTestExpect(withSlash.FilesBaseUrl() == "https://pub-x.r2.dev/files/", withSlash.FilesBaseUrl());
                DistSelfTestExpect(withoutSlash.FilesBaseUrl() == withSlash.FilesBaseUrl(), withoutSlash.FilesBaseUrl());
                DistSelfTestExpect(withSlash.ManifestUrl() == "https://pub-x.r2.dev/manifest.json", withSlash.ManifestUrl());
                DistSelfTestExpect(DistConfig{ "", "", "rclone" }.FilesBaseUrl().empty(), "empty");
                r.Ok("baseUrl is <public>/files/ with or without a trailing slash");
            });
        }

        void StageRefs(DistSelfTestReporter& r)
        {
            r.Section("stage 7: .efkefc / .mv1 referencing files that are not shipped");
            r.Run("unshipped refs", [&]
            {
                using Row = std::tuple<std::string, std::string, std::string>;
                const auto rows = [](const DistRefReport& report)
                {
                    std::vector<Row> result;
                    for (const DistUnshippedRef& item : report.unshipped)
                        result.emplace_back(item.path, item.ref, item.reason);
                    std::ranges::sort(result);
                    return result;
                };

                const DistSelfTestTempDir tmp;
                const std::filesystem::path& root = tmp.Path();
                DistSelfTestMakeRefTree(root);
                const std::filesystem::path cachePath = root / L"cache.json";

                DistHashCache first(cachePath);
                const DistScanResult result = ScanAssets(root / L"Assets", root, first);
                const DistRefReport  report = FindUnshippedRefs(result, root, root / L"Assets", first);
                first.Save();
                std::vector<Row> expected =
                {
                    { "Assets/Art/Effect/Foo/Foo.efkefc", "../../../../Outside/tex.png", DIST_REASON_OUTSIDE },
                    { "Assets/Art/Effect/Foo/Foo.efkefc", "../_Source/Foo/src.png", DIST_REASON_EXCLUDED },
                    { "Assets/Art/Models/M.mv1", "..\\..\\..\\Outside\\tex.png", DIST_REASON_OUTSIDE },
                };
                std::ranges::sort(expected);
                const std::vector<Row> got = rows(report);
                std::vector<std::string> gotText;
                for (const auto& [path, ref, reason] : got)
                    gotText.push_back(path + " " + ref + " " + reason);
                DistSelfTestExpect(got == expected, DistSelfTestJoin(gotText));
                r.Ok("outside Assets/ and excluded targets are reported; shipped and missing ones are not");

                DistSelfTestExpect(report.unreadable == std::vector<std::string>{ "Assets/Art/Models/Broken.mv1" }, DistSelfTestJoin(report.unreadable));
                r.Ok("an unparsable .mv1 is listed as unreadable instead of failing");

                DistSelfTestExpect(first.refMisses == 3 && first.refHits == 0, std::format("{} / {}", first.refMisses, first.refHits));
                DistHashCache second(cachePath);
                const DistRefReport again = FindUnshippedRefs(ScanAssets(root / L"Assets", root, second), root, root / L"Assets", second);
                DistSelfTestExpect(second.refMisses == 0 && second.refHits == 3, std::format("{} / {}", second.refMisses, second.refHits));
                DistSelfTestExpect(rows(again) == got && again.unreadable == report.unreadable, "warm run differs");
                r.Ok("warm run reads no references (cached by content hash, unreadable included)");

                // 参照キャッシュを足す前の形式 (パス -> [mtime, size, sha256] の平たいオブジェクト)
                CEREAL_RAPIDJSON_NAMESPACE::Document saved;
                const std::string savedText = DistSelfTestRead(cachePath);
                saved.Parse(savedText.c_str(), savedText.size());
                CEREAL_RAPIDJSON_NAMESPACE::StringBuffer buffer;
                CEREAL_RAPIDJSON_NAMESPACE::Writer<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer> writer(buffer);
                saved["hashes"].Accept(writer);
                DistSelfTestWrite(root, "legacy.json", std::string(buffer.GetString(), buffer.GetSize()));
                DistHashCache legacy(root / L"legacy.json");
                static_cast<void>(ScanAssets(root / L"Assets", root, legacy));
                DistSelfTestExpect(legacy.misses == 0 && legacy.hits == first.misses, std::format("{} / {}", legacy.misses, legacy.hits));
                r.Ok("a cache file from before the refs cache still provides hashes");
            });

            r.Run("efkefc / mv1 readers", [&]
            {
                const std::optional<std::vector<std::string>> efkefc = EfkefcAssetPaths(DistSelfTestEfkefc({ "Texture/a.png", "道のテクスチャ/b.png" }));
                DistSelfTestExpect(efkefc && *efkefc == std::vector<std::string>{ "Texture/a.png", "道のテクスチャ/b.png" }, "efkefc");
                DistSelfTestExpect(!EfkefcAssetPaths("not an effect"), "not efkefc");

                std::string garbled = DistSelfTestEfkefc({ "Texture/a.png" });
                garbled += "junk";
                const std::string header = garbled.substr(0, 12);
                std::string info = garbled.substr(16);
                std::string rebuilt = header;
                DistSelfTestAppendI32(rebuilt, static_cast<std::int32_t>(info.size()));
                rebuilt += info;
                const std::optional<std::vector<std::string>> fallback = EfkefcAssetPaths(rebuilt);
                DistSelfTestExpect(fallback && *fallback == std::vector<std::string>{ "Texture/a.png" }, "fallback: " + (fallback ? DistSelfTestJoin(*fallback) : std::string("nullopt")));
                r.Ok(".efkefc INFO is parsed, and an unfamiliar layout falls back to scanning the text");

                const std::optional<std::vector<std::string>> mv1 = Mv1TexturePaths(DistSelfTestMv1({ "a.PNG", "note.txt", "a.PNG", std::string(300, 'x') + ".png" }));
                DistSelfTestExpect(mv1 && mv1->size() == 2 && (*mv1)[0] == "a.PNG" && (*mv1)[1] == std::string(259, 'x') + ".png",
                                   mv1 ? DistSelfTestJoin(*mv1) : std::string("nullopt"));
                r.Ok(".mv1 texture strings: case-insensitive, deduplicated, at most 259 bytes before the extension");

                // キーバイト 0x01 で "abcd" を 4 回 (距離 4 の重なる一致) にしたもの
                std::string lz = "MV11";
                DistSelfTestAppendI32(lz, 16);
                DistSelfTestAppendI32(lz, 9 + 7);
                lz.push_back('\x01');
                lz += "abcd";
                lz.push_back('\x01');
                // code: (12 - 4) << 3 = 0x40 (+1 はキーより大きいため)、距離 - 1 = 3
                lz.push_back(static_cast<char>(0x41));
                lz.push_back('\x03');
                const std::optional<std::string> decoded = Mv1Decode(lz);
                DistSelfTestExpect(decoded == std::string("abcdabcdabcdabcd"), decoded.value_or("nullopt"));
                r.Ok(".mv1 LZ back-references (overlapping) decode");
            });
        }

        void StageLockedFiles(DistSelfTestReporter& r)
        {
            r.Section("stage 8: fonts can't be patched while the game runs");
            r.Run("locked files", [&]
            {
                const AssetManifest live = DistSelfTestManifest({
                    DistSelfTestEntry("Assets/Art/Font/ipam.ttf", "f1"), DistSelfTestEntry("Assets/Art/Font/onryou.ttf", "f2"),
                    DistSelfTestEntry("Assets/Art/Font/old.otf", "f3"), DistSelfTestEntry("Assets/a.png", "p1"),
                });
                const AssetManifest next = DistSelfTestManifest({
                    DistSelfTestEntry("Assets/Art/Font/ipam.ttf", "f1"), DistSelfTestEntry("Assets/Art/Font/onryou.ttf", "F2"),
                    DistSelfTestEntry("Assets/Art/Font/added.ttf", "f4"), DistSelfTestEntry("Assets/a.png", "P1"),
                });
                const std::vector<std::string> changed = LockedFileChanges(live, next);
                DistSelfTestExpect(changed == std::vector<std::string>{ "Assets/Art/Font/old.otf", "Assets/Art/Font/onryou.ttf" }, DistSelfTestJoin(changed));
                r.Ok("changed and removed fonts are caught; added fonts and non-fonts are not");

                DistSelfTestExpect(LockedFileChanges(live, live).empty() && LockedFileChanges(DistSelfTestManifest({}), next).empty(), "no change");
                r.Ok("no change / first release -> nothing to refuse");

                DistSelfTestExpect(IsOlderVersion("1.0.0", "1.1.0") && IsOlderVersion("1.9.0", "1.10.0") && IsOlderVersion("1.0", "1.0.1"), "older");
                DistSelfTestExpect(!IsOlderVersion("1.0.0", "1.0.0") && !IsOlderVersion("1.0.0", "1.0") && !IsOlderVersion("", ""), "not older");
                DistSelfTestExpect(IsValidVersionString("1.2.7") && !IsValidVersionString("") && !IsValidVersionString("1 2"), "version string");
                r.Ok("version comparison is numeric per dot-separated part (1.10 > 1.9)");
            });
        }
    }

    int RunDistSelfTest(const std::function<void(std::string)>& print, const std::filesystem::path& repoRoot)
    {
        DistSelfTestReporter reporter(print);
        StageJsonShape(reporter);
        StageExclusions(reporter);
        StageRealGuid(reporter, repoRoot);
        StageScan(reporter);
        StageDiff(reporter);
        StageHashCache(reporter);
        StageUpload(reporter);
        StageRefs(reporter);
        StageLockedFiles(reporter);
        return reporter.Finish();
    }
}
