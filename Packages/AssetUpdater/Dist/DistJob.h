#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "DistConfig.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    class Rclone;

    enum class DistStep
    {
        None,
        /** Assets/ を走査して manifest.json を書き出す */
        Build,
        /** 何を上げるかの表示と中身の検証だけ行う */
        DryRun,
        /** 参照ファイルを配信先へ上げ、最後に manifest.json を差し替える */
        Release,
        /** 公開中の manifest.json と手元の manifest.json の差分 (クライアントが落とす量) を出す */
        DiffLive,
        SelfTest,
    };

    struct NANAMI_API DistRequest
    {
        DistStep              step = DistStep::None;
        std::string           version;
        /** これ未満のクライアントには更新を当てない */
        std::string           requiredClientVersion;
        DistConfig            config;
        /** Assets/ と manifest.json のあるプロジェクトのルート */
        std::filesystem::path repoRoot;
    };

    /** アセット配信の 1 ステップをワーカースレッドで動かし、出力を行ごとに溜める。終了コードは 0 = 成功、1 = 失敗 */
    class NANAMI_API DistJob final
    {
    public:
        DistJob() = default;
        ~DistJob();
        DistJob(const DistJob&)            = delete;
        DistJob& operator=(const DistJob&) = delete;

        /** 前回の出力を捨てて始める。実行中なら false */
        bool Start(DistRequest request);
        /** 次のファイルの区切りか、実行中の rclone を止めた時点で終わる。終了は待たない */
        void Cancel();

        [[nodiscard]] bool               IsRunning   () const;
        [[nodiscard]] bool               WasCanceled () const;
        /** 実行中と中止のときは空 */
        [[nodiscard]] std::optional<int> ExitCode    () const;
        /** 直近の Start からの経過時間 ("m:ss")。終了後は止まる */
        [[nodiscard]] std::string        ElapsedLabel() const;
        /** from 行目以降を out に足し、全体の行数を返す */
        size_t                           CopyLines   (std::vector<std::string>& out, size_t from) const;

    private:
        void Run(const DistRequest& request);
        int  RunBuild   (const DistRequest& request);
        int  RunUpload  (const DistRequest& request, bool dryRun);
        int  RunDiffLive(const DistRequest& request);
        void Print(std::string line);
        /** Cancel が実行中の rclone を止められるよう登録する */
        void SetActiveRclone(Rclone* rclone);
        [[nodiscard]] bool IsCanceled() const;
        void Join();

        std::thread       worker_;
        std::atomic<bool> running_  = false;
        std::atomic<bool> canceled_ = false;

        mutable std::mutex                    mutex_;
        Rclone*                               rclone_ = nullptr;
        std::vector<std::string>              lines_;
        std::optional<int>                    exitCode_;
        std::chrono::steady_clock::time_point startTime_;
        std::chrono::steady_clock::time_point endTime_;
    };
}
