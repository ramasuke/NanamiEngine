#include "ApplicationConfiguration_CodeEditor.h"

#include <cstdio>
#include <system_error>

#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto DEFAULT_KIND               = CodeEditorKind::Rider;
    constexpr auto DEFAULT_RIDER_PATH         = R"(C:\Program Files\JetBrains\JetBrains Rider 2026.2.1\bin\rider64.exe)";
    constexpr auto DEFAULT_VISUAL_STUDIO_PATH = R"(C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe)";
    constexpr auto DEFAULT_CUSTOM_PATH        = "";
    constexpr auto DEFAULT_CUSTOM_ARGUMENTS   = "{file}";

    CodeEditorKind CodeEditorConfiguration::kind_             = DEFAULT_KIND;
    std::string    CodeEditorConfiguration::riderPath_        = DEFAULT_RIDER_PATH;
    std::string    CodeEditorConfiguration::visualStudioPath_ = DEFAULT_VISUAL_STUDIO_PATH;
    std::string    CodeEditorConfiguration::customPath_       = DEFAULT_CUSTOM_PATH;
    std::string    CodeEditorConfiguration::customArguments_  = DEFAULT_CUSTOM_ARGUMENTS;

    constexpr auto CODE_EDITOR_CONFIG_PATH            = "CodeEditor/";
    constexpr auto CODE_EDITOR_KIND_KEY               = "Kind";
    constexpr auto CODE_EDITOR_RIDER_PATH_KEY         = "RiderPath";
    constexpr auto CODE_EDITOR_VISUAL_STUDIO_PATH_KEY = "VisualStudioPath";
    constexpr auto CODE_EDITOR_CUSTOM_PATH_KEY        = "CustomPath";
    constexpr auto CODE_EDITOR_CUSTOM_ARGUMENTS_KEY   = "CustomArguments";

    constexpr auto FILE_PLACEHOLDER = L"{file}";

    namespace
    {
        // ImGui の入力は UTF-8 なので、ACP ではなく UTF-8 として wide 文字列にする
        std::wstring Utf8ToWide(const std::string& utf8)
        {
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end())).wstring();
        }

        std::wstring Quote(const std::filesystem::path& file)
        {
            return L"\"" + file.wstring() + L"\"";
        }

        /** @brief std::string を InputText で編集し、編集が確定したフレームで true を返す */
        bool InputString(const char* label, std::string& value)
        {
            char buffer[512] = {};
            snprintf(buffer, sizeof(buffer), "%s", value.c_str());
            if (ImGui::InputText(label, buffer, sizeof(buffer)))
            {
                value = buffer;
            }
            return ImGui::IsItemDeactivatedAfterEdit();
        }
    }

    void CodeEditorConfiguration::Load()
    {
        kind_             = Module::ProjectConfig::LoadOrDefaultWithPath<CodeEditorKind>(CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_KIND_KEY,               DEFAULT_KIND);
        riderPath_        = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_RIDER_PATH_KEY,         std::string(DEFAULT_RIDER_PATH));
        visualStudioPath_ = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_VISUAL_STUDIO_PATH_KEY, std::string(DEFAULT_VISUAL_STUDIO_PATH));
        customPath_       = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_CUSTOM_PATH_KEY,        std::string(DEFAULT_CUSTOM_PATH));
        customArguments_  = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_CUSTOM_ARGUMENTS_KEY,   std::string(DEFAULT_CUSTOM_ARGUMENTS));
    }

    void CodeEditorConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<CodeEditorKind>(CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_KIND_KEY,               kind_);
        Module::ProjectConfig::SaveWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_RIDER_PATH_KEY,         riderPath_);
        Module::ProjectConfig::SaveWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_VISUAL_STUDIO_PATH_KEY, visualStudioPath_);
        Module::ProjectConfig::SaveWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_CUSTOM_PATH_KEY,        customPath_);
        Module::ProjectConfig::SaveWithPath<std::string>   (CODE_EDITOR_CONFIG_PATH, CODE_EDITOR_CUSTOM_ARGUMENTS_KEY,   customArguments_);
    }

    const char* CodeEditorConfiguration::DisplayName()
    {
        switch (kind_)
        {
        case CodeEditorKind::Rider:        return "Rider";
        case CodeEditorKind::VisualStudio: return "Visual Studio";
        case CodeEditorKind::Custom:       return "Custom Editor";
        }
        return "Custom Editor";
    }

    std::string& CodeEditorConfiguration::SelectedExecutablePath()
    {
        switch (kind_)
        {
        case CodeEditorKind::Rider:        return riderPath_;
        case CodeEditorKind::VisualStudio: return visualStudioPath_;
        case CodeEditorKind::Custom:       return customPath_;
        }
        return customPath_;
    }

    std::filesystem::path CodeEditorConfiguration::ExecutablePath()
    {
        return Utf8ToWide(SelectedExecutablePath());
    }

    std::wstring CodeEditorConfiguration::BuildArguments(const std::filesystem::path& file)
    {
        switch (kind_)
        {
        case CodeEditorKind::Rider:        return Quote(file);
        case CodeEditorKind::VisualStudio: return L"/Edit " + Quote(file);
        case CodeEditorKind::Custom:       break;
        }

        const std::wstring placeholder = FILE_PLACEHOLDER;
        const std::wstring quotedFile  = Quote(file);
        std::wstring arguments = Utf8ToWide(customArguments_);
        for (size_t pos = arguments.find(placeholder); pos != std::wstring::npos; pos = arguments.find(placeholder, pos + quotedFile.size()))
        {
            arguments.replace(pos, placeholder.size(), quotedFile);
        }
        return arguments;
    }

    void CodeEditorConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Open source files from Project Window with");
        ImGui::Separator();

        int kindIndex = static_cast<int>(kind_);
        bool kindChanged = ImGui::RadioButton("Rider", &kindIndex, static_cast<int>(CodeEditorKind::Rider));
        ImGui::SameLine();
        kindChanged |= ImGui::RadioButton("Visual Studio", &kindIndex, static_cast<int>(CodeEditorKind::VisualStudio));
        ImGui::SameLine();
        kindChanged |= ImGui::RadioButton("Custom", &kindIndex, static_cast<int>(CodeEditorKind::Custom));
        if (kindChanged)
        {
            kind_ = static_cast<CodeEditorKind>(kindIndex);
            Save();
        }

        ImGui::Spacing();
        ImGui::Text("Executable");
        ImGui::SetNextItemWidth(450);
        if (InputString("##CodeEditorExecutable", SelectedExecutablePath()))
        {
            Save();
        }

        if (std::error_code ec; !std::filesystem::exists(ExecutablePath(), ec))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Executable not found");
        }

        if (kind_ == CodeEditorKind::Custom)
        {
            ImGui::Spacing();
            ImGui::Text("Arguments");
            ImGui::SetNextItemWidth(450);
            if (InputString("##CodeEditorArguments", customArguments_))
            {
                Save();
            }
            ImGui::TextDisabled("* {file} is replaced with the quoted file path");
        }
    }
}
