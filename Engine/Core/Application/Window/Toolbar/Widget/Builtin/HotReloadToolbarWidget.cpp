#include "HotReloadToolbarWidget.h"

#include <fstream>
#include <string>

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../../HotReload/GameModule.h"
#include "../../../../Configuration/Build/ApplicationConfiguration_Build.h"
#include "../../../../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Toolbar
{
    namespace
    {
        std::string PathToUtf8(const std::filesystem::path& path)
        {
            const std::u8string u8 = path.u8string();
            return std::string(u8.begin(), u8.end());
        }

        /** @brief 作業ディレクトリ直下の .sln がちょうど 1 つならそれを返す */
        std::filesystem::path FindSolution()
        {
            std::error_code ec;
            std::filesystem::path found;
            int count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path(ec), ec))
            {
                if (entry.is_regular_file(ec) && entry.path().extension() == L".sln")
                {
                    found = entry.path();
                    ++count;
                }
            }
            return count == 1 ? found : std::filesystem::path();
        }

        /** @brief .sln の中でエンジン (NanamiEngine / NanamiHost) 以外の .vcxproj がちょうど 1 つならそれ = ゲームプロジェクト */
        std::filesystem::path FindGameProject(const std::filesystem::path& solution)
        {
            std::ifstream stream(solution);
            std::string   line;
            std::filesystem::path found;
            int count = 0;
            while (std::getline(stream, line))
            {
                // Project("{...}") = "Name", "Path\Name.vcxproj", "{guid}"
                if (line.rfind("Project(", 0) != 0)
                    continue;
                const std::size_t equal = line.find('=');
                if (equal == std::string::npos)
                    continue;
                const std::size_t first = line.find('"', line.find(',', equal) + 1);
                const std::size_t last  = first == std::string::npos ? std::string::npos : line.find('"', first + 1);
                if (first == std::string::npos || last == std::string::npos)
                    continue;
                const std::string relative = line.substr(first + 1, last - first - 1);
                const std::filesystem::path path = solution.parent_path() / std::filesystem::path(std::u8string(relative.begin(), relative.end()));
                if (path.extension() != L".vcxproj")
                    continue;
                const std::wstring name = path.stem().wstring();
                if (name == L"NanamiEngine" || name == L"NanamiHost")
                    continue;
                found = path;
                ++count;
            }
            return count == 1 ? found : std::filesystem::path();
        }

        const wchar_t* RunningConfigurationName()
        {
#ifdef _DEBUG
            return L"Debug";
#else
            return L"Release";
#endif
        }
    }

    bool HotReloadToolbarWidget::IsVisible() const
    {
        return Application::HotReload::GameModule::Instance().IsLoaded() || buildPending_;
    }

    void HotReloadToolbarWidget::OnDraw(EditorToolbarWidgetContext&)
    {
        auto& gameModule = Application::HotReload::GameModule::Instance();
        PollBuild();

        if (buildPending_)
        {
            ImGui::TextDisabled("Building %s", process_.ElapsedLabel().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Cancel##HotReload"))
                process_.Cancel();
        }
        else
        {
            if (ImGui::Button("Build & Reload"))
                BeginBuild();
            ImGui::SameLine();
            if (ImGui::Button("Reload"))
                gameModule.RequestReload();
        }
        ImGui::SameLine();
        bool keepOld = gameModule.KeepOldModules();
        if (ImGui::Checkbox("Keep old DLL", &keepOld))
            gameModule.SetKeepOldModules(keepOld);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("古い DLL を FreeLibrary しない保険モード。掃除漏れがあっても落ちない。\n外して落ちる箇所があれば docs/HotReload.md §5 の掃除漏れ");
        if (!gameModule.LastReport().empty() && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", gameModule.LastReport().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("gen %d", gameModule.Generation());
        if (ImGui::IsItemHovered() && !gameModule.LastReport().empty())
            ImGui::SetTooltip("%s", gameModule.LastReport().c_str());
    }

    void HotReloadToolbarWidget::BeginBuild()
    {
        const std::filesystem::path msBuild = Application::Configuration::BuildConfiguration::MsBuildPath();
        std::error_code ec;
        if (!std::filesystem::is_regular_file(msBuild, ec))
        {
            Module::LogError("HotReload: MSBuild が見つかりません。Build Settings で設定してください: " + PathToUtf8(msBuild));
            return;
        }
        const std::filesystem::path solution = FindSolution();
        if (solution.empty())
        {
            Module::LogError("HotReload: 作業ディレクトリに .sln がちょうど 1 つある必要があります");
            return;
        }
        const std::filesystem::path gameProject = FindGameProject(solution);
        if (gameProject.empty())
        {
            Module::LogError("HotReload: .sln の中にゲームの .vcxproj (NanamiEngine / NanamiHost 以外) がちょうど 1 つある必要があります: " + PathToUtf8(solution));
            return;
        }

        const std::filesystem::path logDirectory = std::filesystem::current_path(ec) / L"Logs" / L"HotReload";
        std::filesystem::create_directories(logDirectory, ec);
        logPath_      = logDirectory / L"GameBuild.log";
        errorLogPath_ = logDirectory / L"GameBuild.errors.log";

        // ゲームプロジェクトだけを組む (BuildProjectReferences=false)。ロード中の NanamiEngine.dll は差し替えられないので
        // エンジンは触らず、NanamiHotReloadBuild で props のコピーを止め、エンジンが別途再ビルドされていればエラーにする
        const std::wstring commandLine = L"\"" + msBuild.wstring() + L"\" \"" + gameProject.wstring() + L"\""
            L" -p:Configuration=" + std::wstring(RunningConfigurationName()) + L" -p:Platform=x64 -p:PreferredToolArchitecture=x64"
            L" -p:BuildProjectReferences=false -p:NanamiHotReloadBuild=true"
            L" -m -nologo -nodeReuse:false -noConsoleLogger"
            L" \"-flp:LogFile=" + logPath_.wstring() + L";Verbosity=minimal;Encoding=UTF-8\""
            L" \"-flp1:LogFile=" + errorLogPath_.wstring() + L";ErrorsOnly;Encoding=UTF-8\"";
        if (!process_.Start(commandLine, std::filesystem::current_path(ec)))
        {
            Module::LogError("HotReload: MSBuild を起動できませんでした");
            return;
        }
        buildPending_ = true;
        Module::Log("HotReload: ゲーム DLL のビルドを開始しました (" + PathToUtf8(RunningConfigurationName()) + ")");
    }

    void HotReloadToolbarWidget::PollBuild()
    {
        if (!buildPending_ || process_.IsRunning())
            return;
        
        buildPending_ = false;
        if (process_.WasCanceled())
        {
            Module::Log("HotReload: ビルドを中止しました");
            return;
        }
        const std::optional<int> exitCode = process_.ExitCode();
        if (!exitCode || *exitCode != 0)
        {
            LogBuildErrors();
            Module::LogError("HotReload: ビルドに失敗しました (exit code " + (exitCode ? std::to_string(*exitCode) : std::string("?")) + ")。全体のログ: " + PathToUtf8(logPath_));
            return;
        }
        Module::Log("HotReload: ビルド成功 (" + process_.ElapsedLabel() + ")。差し替えます");
        Application::HotReload::GameModule::Instance().RequestReload();
    }

    void HotReloadToolbarWidget::LogBuildErrors() const
    {
        std::ifstream stream(errorLogPath_);
        std::string   line;
        int           shown = 0;
        while (std::getline(stream, line) && shown < 20)
        {
            if (line.empty())
                continue;
            
            Module::LogError(line);
            ++shown;
        }
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(HotReloadToolbarWidget, 250)
}
