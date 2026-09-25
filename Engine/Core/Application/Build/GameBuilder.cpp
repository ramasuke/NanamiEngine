#include "GameBuilder.h"

#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <Windows.h>

#include "../Configuration/Build/ApplicationConfiguration_Build.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../Packages/AssetUpdater/Manifest/InstalledState.h"

namespace NanamiEngine::Core::Application::Build
{
    namespace
    {
        // tools/dist/manifest.py の is_excluded と揃える。ただし .meta は実行時に要るので配る
        constexpr std::wstring_view GAME_BUILD_EXCLUDED_ASSET_DIRECTORIES[]      = { L"assets/scripts" };
        constexpr std::wstring_view GAME_BUILD_EXCLUDED_ASSET_DIRECTORY_NAMES[] = { L"_source" };
        constexpr std::wstring_view GAME_BUILD_EXCLUDED_ASSET_EXTENSIONS[]      = { L".fbx", L".blend", L".blend1", L".efkproj", L".h", L".cpp", L".bak" };
        constexpr std::wstring_view GAME_BUILD_EXCLUDED_ASSET_NAMES[]           = { L"desktop.ini", L"thumbs.db", L".ds_store" };

        constexpr std::wstring_view GAME_BUILD_PACKAGED_PROJECT_CONFIGS[] = { L"ProjectConfig/Application", L"ProjectConfig/Network", L"ProjectConfig/Physics" };

        // AssetUpdatePresenter が作業ディレクトリ直下から読む
        constexpr wchar_t GAME_BUILD_INSTALLED_STATE_FILE[] = L"installed.json";

        constexpr size_t GAME_BUILD_MAX_LOGGED_ERRORS = 50;

        struct GameBuildHandleCloser
        {
            void operator()(const HANDLE handle) const { CloseHandle(handle); }
        };
        using GameBuildHandle = std::unique_ptr<void, GameBuildHandleCloser>;

        std::wstring GameBuildToLower(std::wstring text)
        {
            std::ranges::transform(text, text.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
            return text;
        }

        std::string GameBuildReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream)
            {
                const std::u8string utf8 = path.u8string();
                throw std::runtime_error("ファイルを開けません: " + std::string(utf8.begin(), utf8.end()));
            }
            return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        }

    }

    GameBuilder& GameBuilder::Instance()
    {
        static GameBuilder instance;
        return instance;
    }

    GameBuilder::~GameBuilder()
    {
        Stop();
    }

    bool GameBuilder::Begin(const BuildAction action)
    {
        if (IsBusy())
            return false;
        if (worker_.joinable())
            worker_.join();

        {
            std::lock_guard lock(reportMutex_);
            report_ = BuildReport();
        }
        startTime_ = std::chrono::steady_clock::now();

        try
        {
            using Configuration::BuildConfiguration;

            Paths paths;
            paths.projectRoot    = std::filesystem::current_path();
            paths.outputRoot     = BuildConfiguration::OutputDirectory();
            paths.msBuild        = BuildConfiguration::MsBuildPath();
            paths.configuration  = BuildConfiguration::TargetConfigurationName(BuildConfiguration::TargetConfiguration());
            paths.runAfterBuild  = action == BuildAction::BuildAndRun;
            paths.assetUpdates   = BuildConfiguration::AssetUpdatesEnabled();

            const std::string& productName = BuildConfiguration::ProductName();
            if (const std::string reason = BuildConfiguration::ValidateProductName(productName); !reason.empty())
                return RejectBegin("GameBuilder: 製品名を exe の名前に使えません (" + reason + ")");
            paths.exeFileName = std::filesystem::path(std::u8string(productName.begin(), productName.end()) + u8".exe");

            std::error_code ec;
            if (!BuildConfiguration::StartSceneGuid().empty() && !BuildConfiguration::FindStartSceneFile())
                return RejectBegin("GameBuilder: 起動シーンが見つかりません。Build Settings で選び直してください (guid " + BuildConfiguration::StartSceneGuid() + ")");
            const std::string startScenePath = BuildConfiguration::StartScenePath();
            if (!std::filesystem::is_regular_file(paths.projectRoot / std::filesystem::path(std::u8string(startScenePath.begin(), startScenePath.end())), ec))
                return RejectBegin("GameBuilder: 起動シーンのファイルがありません: " + startScenePath);

            paths.solution = FindSolution(paths.projectRoot);
            if (paths.solution.empty())
                return RejectBegin("GameBuilder: 作業ディレクトリに .sln がちょうど 1 つある必要があります: " + PathToUtf8(paths.projectRoot));
            if (!std::filesystem::is_regular_file(paths.msBuild, ec))
                return RejectBegin("GameBuilder: MSBuild が見つかりません。Build Settings で設定してください: " + PathToUtf8(paths.msBuild));
            // WARNING: 同期は出力先の古いファイルを消すので、プロジェクトと重なる出力先は受け付けない
            if (IsSameOrInside(paths.projectRoot, paths.outputRoot)
                || IsSameOrInside(paths.outputRoot, paths.projectRoot / L"Assets")
                || IsSameOrInside(paths.outputRoot, paths.projectRoot / L"ProjectConfig"))
                return RejectBegin("GameBuilder: 出力先にはプロジェクト・Assets・ProjectConfig と重ならないフォルダを指定してください: " + PathToUtf8(paths.outputRoot));

            cancelRequested_ = false;
            phase_           = Phase::Compiling;
            Module::Log("GameBuilder: Game 版 (" + PathToUtf8(paths.configuration) + ") のビルドを開始しました: " + PathToUtf8(paths.solution));
            worker_ = std::thread([this, paths]() { Run(paths); });
            return true;
        }
        catch (const std::exception& exception)
        {
            phase_ = Phase::Idle;
            return RejectBegin("GameBuilder: ビルドを開始できませんでした: " + std::string(exception.what()));
        }
    }

    void GameBuilder::Cancel()
    {
        if (IsBusy())
            cancelRequested_ = true;
    }

    void GameBuilder::Stop()
    {
        cancelRequested_ = true;
        if (worker_.joinable())
            worker_.join();
    }

    bool GameBuilder::IsBusy() const
    {
        return phase_ != Phase::Idle;
    }

    const char* GameBuilder::PhaseLabel() const
    {
        switch (phase_.load())
        {
        case Phase::Idle:      return "Idle";
        case Phase::Compiling: return "Compiling";
        case Phase::Packaging: return "Packaging";
        }
        return "Idle";
    }

    std::string GameBuilder::ElapsedLabel() const
    {
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime_).count();
        char buffer[32] = {};
        snprintf(buffer, sizeof(buffer), "%d:%02d", static_cast<int>(seconds / 60), static_cast<int>(seconds % 60));
        return buffer;
    }

    BuildReport GameBuilder::LastReport() const
    {
        std::lock_guard lock(reportMutex_);
        return report_;
    }

    void GameBuilder::ReportError(const std::string& message)
    {
        Module::LogError(message);
        std::lock_guard lock(reportMutex_);
        report_.errors.push_back(message);
    }

    bool GameBuilder::RejectBegin(const std::string& message)
    {
        ReportError(message);
        std::lock_guard lock(reportMutex_);
        report_.outcome = BuildOutcome::Failed;
        report_.elapsed = ElapsedLabel();
        return false;
    }

    void GameBuilder::Run(const Paths& paths)
    {
        StepResult result = StepResult::Failed;
        try
        {
            phase_ = Phase::Compiling;
            result = RunMsBuild(paths);
            if (result == StepResult::Succeeded)
            {
                phase_ = Phase::Packaging;
                result = Package(paths);
            }
        }
        catch (const std::exception& exception)
        {
            ReportError("GameBuilder: " + std::string(exception.what()));
            result = StepResult::Failed;
        }

        const std::filesystem::path exePath = paths.outputRoot / paths.exeFileName;
        const std::string           elapsed = ElapsedLabel();
        if (result == StepResult::Succeeded)
        {
            Module::Log("GameBuilder: ゲームのビルドが完了しました: " + PathToUtf8(exePath) + " (" + elapsed + ")");
            if (paths.runAfterBuild)
                LaunchGame(paths);
        }
        else if (result == StepResult::Canceled)
        {
            Module::Log("GameBuilder: ビルドを中止しました");
        }

        {
            std::lock_guard lock(reportMutex_);
            report_.elapsed = elapsed;
            report_.exePath = result == StepResult::Succeeded ? PathToUtf8(exePath) : std::string();
            switch (result)
            {
            case StepResult::Succeeded: report_.outcome = BuildOutcome::Succeeded; break;
            case StepResult::Failed:    report_.outcome = BuildOutcome::Failed;    break;
            case StepResult::Canceled:  report_.outcome = BuildOutcome::Canceled;  break;
            }
        }
        phase_ = Phase::Idle;
    }

    void GameBuilder::LaunchGame(const Paths& paths)
    {
        const std::filesystem::path exePath          = paths.outputRoot / paths.exeFileName;
        const std::wstring          workingDirectory = paths.outputRoot.wstring();
        std::wstring                commandLine      = L"\"" + exePath.wstring() + L"\"";

        // ゲームは Assets/ と ProjectConfig/ を作業ディレクトリから読む
        STARTUPINFOW        startupInfo = {};
        PROCESS_INFORMATION processInfo = {};
        startupInfo.cb = sizeof(startupInfo);
        if (!CreateProcessW(exePath.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0,
                            nullptr, workingDirectory.c_str(), &startupInfo, &processInfo))
        {
            ReportError("GameBuilder: ビルドしたゲームを起動できませんでした (GetLastError " + std::to_string(GetLastError()) + "): " + PathToUtf8(exePath));
            return;
        }
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        Module::Log("GameBuilder: ビルドしたゲームを起動しました: " + PathToUtf8(exePath));
    }

    GameBuilder::StepResult GameBuilder::RunMsBuild(const Paths& paths)
    {
        const std::filesystem::path logDirectory = BuildLogDirectory(paths);
        const std::filesystem::path logPath      = logDirectory / L"GameBuild.log";
        const std::filesystem::path errorLogPath = logDirectory / L"GameBuild.errors.log";
        std::filesystem::create_directories(logDirectory);

        // コンソール出力は CP932 なので受け取らず、UTF-8 のファイルログから読む
        std::wstring commandLine = L"\"" + paths.msBuild.wstring() + L"\" \"" + paths.solution.wstring() + L"\""
            L" -p:Configuration=" + paths.configuration + L" -p:Platform=x64 -p:PreferredToolArchitecture=x64"
            L" -p:NanamiApplicationMode=Game"
            L" -m -nologo -nodeReuse:false -noConsoleLogger"
            L" \"-flp:LogFile=" + logPath.wstring() + L";Verbosity=minimal;Encoding=UTF-8\""
            L" \"-flp1:LogFile=" + errorLogPath.wstring() + L";ErrorsOnly;Encoding=UTF-8\"";

        // エディタが落ちても MSBuild と cl.exe が残らないよう、Job に入れておく
        const GameBuildHandle job(CreateJobObjectW(nullptr, nullptr));
        if (!job)
        {
            ReportError("GameBuilder: Job Object を作れませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
            return StepResult::Failed;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit = {};
        limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limit, sizeof(limit));

        STARTUPINFOW        startupInfo = {};
        PROCESS_INFORMATION processInfo = {};
        startupInfo.cb = sizeof(startupInfo);
        const std::wstring workingDirectory = paths.projectRoot.wstring();
        if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                            nullptr, workingDirectory.c_str(), &startupInfo, &processInfo))
        {
            ReportError("GameBuilder: MSBuild を起動できませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
            return StepResult::Failed;
        }
        const GameBuildHandle process(processInfo.hProcess);
        const GameBuildHandle thread (processInfo.hThread);

        if (!AssignProcessToJobObject(job.get(), process.get()))
        {
            TerminateProcess(process.get(), 1);
            ReportError("GameBuilder: MSBuild を Job に入れられませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
            return StepResult::Failed;
        }
        ResumeThread(thread.get());

        while (WaitForSingleObject(process.get(), 100) == WAIT_TIMEOUT)
        {
            if (cancelRequested_)
            {
                TerminateJobObject(job.get(), 1);
                WaitForSingleObject(process.get(), INFINITE);
                return StepResult::Canceled;
            }
        }

        // 正常に終わった後は、cl.exe が起動した mspdbsrv.exe などを Job を閉じるときに巻き込まない
        limit.BasicLimitInformation.LimitFlags = 0;
        SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limit, sizeof(limit));

        DWORD exitCode = 1;
        GetExitCodeProcess(process.get(), &exitCode);
        if (exitCode != 0)
        {
            LogBuildErrors(errorLogPath, paths);
            ReportError("GameBuilder: ビルドに失敗しました (exit code " + std::to_string(exitCode) + ")。全体のログ: " + PathToUtf8(logPath));
            return StepResult::Failed;
        }
        return StepResult::Succeeded;
    }

    GameBuilder::StepResult GameBuilder::Package(const Paths& paths)
    {
        const std::filesystem::path builtExe = FindBuiltExe(paths);
        if (builtExe.empty())
        {
            ReportError("GameBuilder: ビルドした exe が見つかりません: " + PathToUtf8(paths.projectRoot / L"x64" / L"Game" / paths.configuration));
            return StepResult::Failed;
        }

        std::filesystem::create_directories(paths.outputRoot);
        try
        {
            CopyIfChanged(builtExe, paths.outputRoot / paths.exeFileName);
            // exe の隣の DLL も持っていく (/MD のときの VC++ ランタイム DLL。NanamiEngine.Game.props の NanamiCopyCrtRedist が置く)
            for (const auto& entry : std::filesystem::directory_iterator(builtExe.parent_path()))
            {
                if (entry.is_regular_file() && entry.path().extension() == L".dll")
                    CopyIfChanged(entry.path(), paths.outputRoot / entry.path().filename());
            }
        }
        catch (const std::filesystem::filesystem_error& exception)
        {
            ReportError("GameBuilder: exe をコピーできませんでした。前回ビルドしたゲームが起動中なら閉じてください: " + std::string(exception.what()));
            return StepResult::Failed;
        }

        MirrorStats stats;
        if (const auto result = MirrorDirectory(paths.projectRoot, paths.outputRoot, L"Assets", IsPackagedAsset, stats); result != StepResult::Succeeded)
            return result;
        for (const auto directory : GAME_BUILD_PACKAGED_PROJECT_CONFIGS)
        {
            if (const auto result = MirrorDirectory(paths.projectRoot, paths.outputRoot, directory, [](const std::wstring&) { return true; }, stats); result != StepResult::Succeeded)
                return result;
        }
        // 製品名と起動シーン。ProjectConfig/Build/ 直下の MSBuild のパスなどはエディタ専用なので配らない
        if (const auto result = MirrorDirectory(paths.projectRoot, paths.outputRoot, Configuration::BuildConfiguration::RuntimeConfigDirectory(), [](const std::wstring&) { return true; }, stats); result != StepResult::Succeeded)
            return result;

        Module::Log("GameBuilder: アセットと設定を同期しました (コピー " + std::to_string(stats.copied) + " 件 / 削除 " + std::to_string(stats.removed) + " 件)");
        return WriteAssetUpdateState(paths);
    }

    GameBuilder::StepResult GameBuilder::WriteAssetUpdateState(const Paths& paths)
    {
        const std::filesystem::path installedState = paths.outputRoot / GAME_BUILD_INSTALLED_STATE_FILE;
        if (!paths.assetUpdates)
        {
            // 前回のビルドの installed.json が残っていると、無効にしたはずの更新が走る
            std::filesystem::remove(installedState);
            return StepResult::Succeeded;
        }

        const AssetUpdater::InstalledStateResult result = AssetUpdater::InstalledStateWriter(paths.outputRoot, installedState)
            .Write([this] { return cancelRequested_.load(); });
        if (result.canceled)
            return StepResult::Canceled;
        if (!result.ok)
        {
            ReportError("GameBuilder: installed.json を書けませんでした: " + result.error);
            return StepResult::Failed;
        }
        Module::Log("GameBuilder: installed.json を書きました (" + std::to_string(result.entryCount) + " 件)");
        return StepResult::Succeeded;
    }

    GameBuilder::StepResult GameBuilder::MirrorDirectory(const std::filesystem::path& sourceRoot, const std::filesystem::path& destinationRoot,
                                                         const std::filesystem::path& relativeDirectory, const MirrorFilter& filter, MirrorStats& stats) const
    {
        const std::filesystem::path source      = sourceRoot      / relativeDirectory;
        const std::filesystem::path destination = destinationRoot / relativeDirectory;

        std::error_code ec;
        if (!std::filesystem::is_directory(source, ec))
            return StepResult::Succeeded;

        std::unordered_set<std::wstring> keptKeys;
        for (const auto& file : CollectRegularFiles(source))
        {
            if (cancelRequested_)
                return StepResult::Canceled;

            const std::filesystem::path relative = relativeDirectory / file.lexically_relative(source);
            const std::wstring          key      = relative.generic_wstring();
            if (!filter(key))
                continue;

            // 大文字小文字だけ変わったファイルを、コピー直後に古い名前として消さないよう小文字で持つ
            keptKeys.insert(GameBuildToLower(key));
            if (CopyIfChanged(file, destinationRoot / relative))
                ++stats.copied;
        }

        if (!std::filesystem::is_directory(destination, ec))
            return StepResult::Succeeded;

        for (const auto& file : CollectRegularFiles(destination))
        {
            if (cancelRequested_)
                return StepResult::Canceled;

            const std::wstring key = (relativeDirectory / file.lexically_relative(destination)).generic_wstring();
            if (filter(key) && !keptKeys.contains(GameBuildToLower(key)))
            {
                std::filesystem::remove(file);
                ++stats.removed;
            }
        }
        return StepResult::Succeeded;
    }

    std::vector<std::filesystem::path> GameBuilder::CollectRegularFiles(const std::filesystem::path& root)
    {
        std::vector<std::filesystem::path> files;
        for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            if (it->is_directory())
            {
                // ジャンクションの先を同期や削除の対象にしない
                if (GetFileAttributesW(it->path().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT)
                    it.disable_recursion_pending();
                continue;
            }
            if (it->is_regular_file())
                files.push_back(it->path());
        }
        return files;
    }

    bool GameBuilder::CopyIfChanged(const std::filesystem::path& source, const std::filesystem::path& destination)
    {
        const auto sourceSize = std::filesystem::file_size(source);
        const auto sourceTime = std::filesystem::last_write_time(source);

        std::error_code sizeError;
        std::error_code timeError;
        if (std::filesystem::file_size(destination, sizeError) == sourceSize && !sizeError
            && std::filesystem::last_write_time(destination, timeError) == sourceTime && !timeError)
            return false;

        std::filesystem::create_directories(destination.parent_path());
        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing);
        // MSBuild の差分ビルドは更新日時で判断するので、元のファイルに揃える
        std::filesystem::last_write_time(destination, sourceTime);
        return true;
    }

    bool GameBuilder::IsPackagedAsset(const std::wstring& projectRelativePath)
    {
        const std::wstring lowered   = GameBuildToLower(projectRelativePath);
        const size_t       lastSlash = lowered.find_last_of(L'/');
        const std::wstring name      = lastSlash == std::wstring::npos ? lowered : lowered.substr(lastSlash + 1);

        if (std::ranges::any_of(GAME_BUILD_EXCLUDED_ASSET_NAMES, [&name](const std::wstring_view excluded) { return name == excluded; }))
            return false;
        if (std::ranges::any_of(GAME_BUILD_EXCLUDED_ASSET_EXTENSIONS, [&lowered](const std::wstring_view extension) { return lowered.ends_with(extension); }))
            return false;
        for (size_t begin = 0, end = lowered.find(L'/'); end != std::wstring::npos; begin = end + 1, end = lowered.find(L'/', begin))
        {
            const std::wstring_view directoryName = std::wstring_view(lowered).substr(begin, end - begin);
            if (std::ranges::any_of(GAME_BUILD_EXCLUDED_ASSET_DIRECTORY_NAMES, [directoryName](const std::wstring_view excluded) { return directoryName == excluded; }))
                return false;
        }
        return !std::ranges::any_of(GAME_BUILD_EXCLUDED_ASSET_DIRECTORIES, [&lowered](const std::wstring_view directory)
        {
            return lowered == directory || lowered.starts_with(std::wstring(directory) + L'/');
        });
    }

    bool GameBuilder::IsSameOrInside(const std::filesystem::path& path, const std::filesystem::path& base)
    {
        const auto normalize = [](const std::filesystem::path& target)
        {
            std::filesystem::path normalized = std::filesystem::weakly_canonical(target);
            // "Build/Game/" のような末尾の区切りが空の要素として残ると、比較が素通りしてしまう
            if (!normalized.has_filename() && normalized.has_relative_path())
                normalized = normalized.parent_path();
            return normalized;
        };

        const std::filesystem::path normalizedPath = normalize(path);
        const std::filesystem::path normalizedBase = normalize(base);
        auto pathIt = normalizedPath.begin();
        for (auto baseIt = normalizedBase.begin(); baseIt != normalizedBase.end(); ++baseIt, ++pathIt)
        {
            if (pathIt == normalizedPath.end() || _wcsicmp(baseIt->c_str(), pathIt->c_str()) != 0)
                return false;
        }
        return true;
    }

    std::filesystem::path GameBuilder::FindSolution(const std::filesystem::path& projectRoot)
    {
        std::filesystem::path found;
        std::error_code       ec;
        for (const auto& entry : std::filesystem::directory_iterator(projectRoot, ec))
        {
            if (!entry.is_regular_file() || GameBuildToLower(entry.path().extension().wstring()) != L".sln")
                continue;
            if (!found.empty())
                return {};
            found = entry.path();
        }
        return found;
    }

    std::filesystem::path GameBuilder::FindBuiltExe(const Paths& paths)
    {
        std::filesystem::path           newest;
        std::filesystem::file_time_type newestTime;
        std::error_code                 ec;
        for (const auto& entry : std::filesystem::directory_iterator(paths.projectRoot / L"x64" / L"Game" / paths.configuration, ec))
        {
            if (!entry.is_regular_file() || GameBuildToLower(entry.path().extension().wstring()) != L".exe")
                continue;
            const auto time = entry.last_write_time();
            if (newest.empty() || time > newestTime)
            {
                newest     = entry.path();
                newestTime = time;
            }
        }
        return newest;
    }

    std::filesystem::path GameBuilder::BuildLogDirectory(const Paths& paths)
    {
        return paths.projectRoot / L"x64" / L"Game";
    }

    std::string GameBuilder::PathToUtf8(const std::filesystem::path& path)
    {
        const std::u8string utf8 = path.u8string();
        return std::string(utf8.begin(), utf8.end());
    }

    void GameBuilder::LogBuildErrors(const std::filesystem::path& errorLogPath, const Paths& paths)
    {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(errorLogPath, ec))
            return;

        std::string text = GameBuildReadAllBytes(errorLogPath);
        if (text.starts_with("\xEF\xBB\xBF"))
            text.erase(0, 3);

        size_t lineCount = 0;
        size_t begin     = 0;
        while (begin < text.size())
        {
            size_t end = text.find('\n', begin);
            if (end == std::string::npos)
                end = text.size();

            std::string line = text.substr(begin, end - begin);
            begin = end + 1;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                continue;

            if (++lineCount <= GAME_BUILD_MAX_LOGGED_ERRORS)
                ReportError("[Build] " + line);
        }
        if (lineCount > GAME_BUILD_MAX_LOGGED_ERRORS)
        {
            Module::LogError("[Build] ...ほか " + std::to_string(lineCount - GAME_BUILD_MAX_LOGGED_ERRORS) + " 行: " + PathToUtf8(errorLogPath));
            std::lock_guard lock(reportMutex_);
            report_.omittedErrorCount = lineCount - GAME_BUILD_MAX_LOGGED_ERRORS;
        }
    }
}
