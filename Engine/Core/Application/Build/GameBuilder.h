#pragma once
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
    struct BuildReport
    {
        BuildOutcome             outcome = BuildOutcome::None;
        std::string              elapsed;
        std::string              exePath;
        std::vector<std::string> errors;
        /** @brief errors に入りきらなかった MSBuild のエラー行の数 */
        size_t                   omittedErrorCount = 0;
    };

    /**
     * @brief APPLICATION_MODE を Game に書き換えたソースを MSBuild でビルドし、配布用フォルダにまとめる。
     *        書き換えたソースを作業ツリーに置くとエディタ側が再ビルドになるので、
     *        その場ではビルドせず、ステージングへ複製してからビルドする
     */
    class GameBuilder final
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
            CopyingSources,
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
        struct Paths
        {
            std::filesystem::path repositoryRoot;
            std::filesystem::path stagingRoot;
            std::filesystem::path outputRoot;
            std::filesystem::path msBuild;
            /** @brief MSBuild の Configuration 名 (Release / Debug) */
            std::wstring          configuration;
            /** @brief 出力する exe のファイル名 (製品名 + ".exe") */
            std::filesystem::path exeFileName;
            bool                  runAfterBuild = false;
        };

        struct MirrorStats
        {
            size_t copied  = 0;
            size_t removed = 0;
        };

        /** @brief リポジトリルートからの相対パス (区切りは '/') を受け取り、同期対象なら true を返す */
        using MirrorFilter = std::function<bool(const std::wstring& repositoryRelativePath)>;

        GameBuilder() = default;

        void       Run           (const Paths& paths);
        StepResult SyncSources   (const Paths& paths) const;
        StepResult ApplyOverrides(const Paths& paths);
        StepResult RunMsBuild    (const Paths& paths);
        StepResult Package       (const Paths& paths);
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

        static bool IsStagedSource  (const std::wstring& repositoryRelativePath);
        static bool IsPackagedAsset (const std::wstring& repositoryRelativePath);
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
