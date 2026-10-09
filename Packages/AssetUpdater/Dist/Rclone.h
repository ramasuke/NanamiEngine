#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "../Manifest/AssetManifest.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    class NANAMI_API DistUploadError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    // NOTE: Cancel で子プロセスを止めたとき
    class NANAMI_API DistCanceledError : public std::runtime_error
    {
    public:
        DistCanceledError() : std::runtime_error("canceled") {}
    };

    // NOTE: rclone を同期で呼ぶ。失敗は DistUploadError、Cancel は DistCanceledError で抜ける
    class NANAMI_API Rclone final
    {
    public:
        using LineSink = std::function<void(const std::string&)>;

        Rclone(std::string exe, const std::string& remote, LineSink log);
        ~Rclone();
        Rclone(const Rclone&)            = delete;
        Rclone& operator=(const Rclone&) = delete;

        // NOTE: files/ にあるブロブのハッシュ
        [[nodiscard]] std::unordered_set<std::string> ListBlobHashes();
        // NOTE: stagingDir の中身を files/ へ上げる。rclone の進捗はログへ流す
        void UploadBlobs(const std::filesystem::path& stagingDir);
        [[nodiscard]] std::unordered_set<std::string> RootFileNames();
        [[nodiscard]] std::string ReadFile(const std::string& remoteName);
        void UploadFile(const std::filesystem::path& local, const std::string& remoteName, const std::string& header);

        // NOTE: いま公開中の manifest.json。まだ一度もリリースしていなければ nullopt
        [[nodiscard]] std::optional<AssetManifest> ReadLiveManifest();
        // NOTE: 同名がすでにあり、中身が違うなら true (rclone の --immutable は copyto では効かないので自前で見る)
        [[nodiscard]] bool VersionedConflicts(const std::filesystem::path& local, const std::string& remoteName);
        // NOTE: 一度置いたら中身を変えさせない。同じ中身なら何もせず false、無ければ上げて true
        bool UploadFileOnce(const std::filesystem::path& local, const std::string& remoteName, const std::string& header);

        // NOTE: 別スレッドから呼んでよい。実行中の rclone を止める
        void Cancel();

    private:
        struct NANAMI_API RunResult
        {
            int         exitCode = 0;
            std::string out;
            std::string err;
        };

        RunResult Run(const std::vector<std::string>& args, bool capture);
        [[nodiscard]] std::string Remote(const std::string& name) const;

        std::string       exe_;
        std::string       remote_;
        LineSink          log_;
        std::atomic<bool> canceled_ = false;
        std::mutex        jobMutex_;
        void*             job_      = nullptr;
    };

    [[nodiscard]] NANAMI_API std::string ReadLocalFile(const std::filesystem::path& path);
}
