#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace NanamiEngine::Core::Application::Build
{
    enum class BuildAction
    {
        Build,
        /** @brief ビルドに成功したら出力した exe を起動する */
        BuildAndRun,
    };

    enum class BuildOutcome
    {
        None,
        Succeeded,
        Failed,
        Canceled,
    };

    /** @brief 直近のビルドの結果。Build Settings ウィンドウに出す */
    struct NANAMI_API BuildReport
    {
        BuildOutcome             outcome = BuildOutcome::None;
        std::string              elapsed;
        std::string              exePath;
        std::vector<std::string> errors;
        /** @brief errors に入りきらなかった MSBuild のエラー行の数 */
        size_t                   omittedErrorCount = 0;
    };

    /** @brief プロジェクトの .sln をゲーム版 (NanamiApplicationMode=Game) で MSBuild し、配布用フォルダにまとめる */
    class NANAMI_API GameBuilder final
    {
    public:
        static GameBuilder& Instance();
        ~GameBuilder();
        GameBuilder(const GameBuilder&)            = delete;
        GameBuilder& operator=(const GameBuilder&) = delete;

        /** @brief Build Settings の内容でワーカースレッドのビルドを始める。ビルド中か事前チェックに失敗したら false */
        bool Begin(BuildAction action);
        /** @brief 中止を要求する。ワーカーの終了は待たない */
        void Cancel();
        /** @brief 中止してワーカーの終了を待つ。エディタ終了時に呼ぶ */
        void Stop();
        [[nodiscard]] bool        IsBusy      () const;
        [[nodiscard]] const char* PhaseLabel  () const;
        /** @brief 直近の Begin からの経過時間 ("m:ss") */
        [[nodiscard]] std::string ElapsedLabel() const;
        /** @brief ビルド中はそこまでに出たエラーが入っている */
        [[nodiscard]] BuildReport LastReport  () const;

    private:
        enum class Phase
        {
            Idle,
            Compiling,
            Packaging,
        };

        enum class StepResult
        {
            Succeeded,
            Failed,
            Canceled,
        };

        /** @brief Begin で取った Build Settings の写し。ビルド中に設定を変えても影響しない */
        struct NANAMI_API Paths
        {
            std::filesystem::path projectRoot;
            std::filesystem::path solution;
            std::filesystem::path outputRoot;
            std::filesystem::path msBuild;
            /** @brief MSBuild の Configuration 名 (Release / Debug) */
            std::wstring          configuration;
            /** @brief 出力する exe のファイル名 (製品名 + ".exe") */
            std::filesystem::path exeFileName;
            bool                  runAfterBuild = false;
            /** @brief 出力先に installed.json を書き、配信中のアセットへの更新を有効にする */
            bool                  assetUpdates  = false;
        };

        struct NANAMI_API MirrorStats
        {
            size_t copied  = 0;
            size_t removed = 0;
        };

        /** @brief プロジェクトルートからの相対パス (区切りは '/') を受け取り、同期対象なら true を返す */
        using MirrorFilter = std::function<bool(const std::wstring& repositoryRelativePath)>;

        GameBuilder() = default;

        void       Run           (const Paths& paths);
        StepResult RunMsBuild    (const Paths& paths);
        StepResult Package       (const Paths& paths);
        StepResult WriteAssetUpdateState(const Paths& paths);
        void       LaunchGame    (const Paths& paths);

        /** @brief ログに出し、直近のビルド結果のエラー一覧にも入れる */
        void ReportError(const std::string& message);
        /** @brief 事前チェックの失敗を報告して false を返す */
        bool RejectBegin(const std::string& message);

        /** @brief source/relativeDirectory を destination/relativeDirectory へ差分同期する。filter に合うのに元に無いファイルは消す */
        StepResult MirrorDirectory(const std::filesystem::path& sourceRoot, const std::filesystem::path& destinationRoot,
                                   const std::filesystem::path& relativeDirectory, const MirrorFilter& filter, MirrorStats& stats) const;
        static std::vector<std::filesystem::path> CollectRegularFiles(const std::filesystem::path& root);
        /** @brief サイズか更新日時が違うときだけコピーし、更新日時を元に揃える。コピーしたら true */
        static bool CopyIfChanged(const std::filesystem::path& source, const std::filesystem::path& destination);

        static bool IsPackagedAsset (const std::wstring& projectRelativePath);
        /** @brief projectRoot 直下の .sln がちょうど 1 つならそれを返す */
        static std::filesystem::path FindSolution(const std::filesystem::path& projectRoot);
        /** @brief ビルドした exe (x64/Game/<Configuration>/ の中で一番新しい .exe) */
        static std::filesystem::path FindBuiltExe(const Paths& paths);
        static std::filesystem::path BuildLogDirectory(const Paths& paths);
        static bool IsSameOrInside  (const std::filesystem::path& path, const std::filesystem::path& base);
        static std::string PathToUtf8(const std::filesystem::path& path);
        void LogBuildErrors(const std::filesystem::path& errorLogPath, const Paths& paths);

        std::thread                           worker_;
        std::atomic<Phase>                    phase_           = Phase::Idle;
        std::atomic<bool>                     cancelRequested_ = false;
        std::chrono::steady_clock::time_point startTime_;
        mutable std::mutex                    reportMutex_;
        BuildReport                           report_;
    };
}
