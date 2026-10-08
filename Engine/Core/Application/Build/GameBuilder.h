#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "Engine/Core/Application/Process/AsyncProcess.h"

namespace NanamiEngine::Core::Application::Build
{
    enum class BuildAction
    {
        Build,
        BuildAndRun,
    };

    enum class BuildOutcome
    {
        None,
        Succeeded,
        Failed,
        Canceled,
    };

    /** @brief 直近のビルドの結果 */
    struct NANAMI_API BuildReport
    {
        BuildOutcome             outcome = BuildOutcome::None;
        std::string              elapsed;
        std::string              exePath;
        std::vector<std::string> errors;
        size_t                   omittedErrorCount = 0;
    };

    class NANAMI_API GameBuilder final
    {
    public:
        static GameBuilder& Instance();
        GameBuilder(const GameBuilder&)            = delete;
        GameBuilder& operator=(const GameBuilder&) = delete;

        /** @brief Build Settings の内容で MSBuild を起動する */
        bool Begin(BuildAction action);
        /** @brief 毎フレーム呼ぶ。MSBuild が終わっていたら、成功時はその場でパッケージまで済ませる*/
        void Update();
        void Cancel();
        [[nodiscard]] bool               IsBusy      () const;
        [[nodiscard]] std::string        ElapsedLabel() const;
        [[nodiscard]] const BuildReport& LastReport  () const;

    private:
        /** @brief Begin で取った Build Settings の写し。ビルド中に設定を変えても影響しない */
        struct NANAMI_API Paths
        {
            std::filesystem::path projectRoot;
            std::filesystem::path solution;
            std::filesystem::path outputRoot;
            std::filesystem::path msBuild;
            /** @brief MSBuild の Configuration (Release / Debug) */
            std::wstring          configuration;
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

        void OnMsBuildFinished();
        void Finish(BuildOutcome outcome);
        bool Package    ();
        bool WriteAssetUpdateState();
        void LaunchGame ();

        void ReportError(const std::string& message);
        bool RejectBegin(const std::string& message);

        /** @brief source/relativeDirectory を destination/relativeDirectory へ差分同期する。filter に合うのに元に無いファイルは消す */
        static void MirrorDirectory(const std::filesystem::path& sourceRoot, const std::filesystem::path& destinationRoot,
                                    const std::filesystem::path& relativeDirectory, const MirrorFilter& filter, MirrorStats& stats);
        static std::vector<std::filesystem::path> CollectRegularFiles(const std::filesystem::path& root);
        /** @brief サイズか更新日時が違うときだけコピーし、更新日時を元に揃える。コピーしたら true */
        static bool CopyIfChanged(const std::filesystem::path& source, const std::filesystem::path& destination);

        static bool IsPackagedAsset(const std::wstring& projectRelativePath);
        static std::filesystem::path FindSolution(const std::filesystem::path& projectRoot);
        static std::filesystem::path FindBuiltExe(const Paths& paths);
        static std::filesystem::path BuildLogDirectory(const Paths& paths);
        static bool IsSameOrInside  (const std::filesystem::path& path, const std::filesystem::path& base);
        static std::string PathToUtf8(const std::filesystem::path& path);
        void LogBuildErrors(const std::filesystem::path& errorLogPath);

        Process::AsyncProcess                 msBuild_;
        bool                                  busy_ = false;
        Paths                                 paths_;
        std::chrono::steady_clock::time_point startTime_;
        BuildReport                           report_;
    };
}
