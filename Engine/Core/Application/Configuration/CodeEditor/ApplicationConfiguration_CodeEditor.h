#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <string>

namespace NanamiEngine::Core::Application::Configuration
{
    enum class CodeEditorKind
    {
        Rider,
        VisualStudio,
        Custom
    };

    /** @brief ProjectWindow からソースファイルを開く外部エディタの設定 */
    class NANAMI_API CodeEditorConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static const char*           DisplayName();
        [[nodiscard]] static std::filesystem::path ExecutablePath();
        [[nodiscard]] static std::wstring          BuildArguments(const std::filesystem::path& file);

        static void DrawConfigGUI();

    private:
        [[nodiscard]] static std::string& SelectedExecutablePath();

        static CodeEditorKind kind_;
        static std::string    riderPath_;
        static std::string    visualStudioPath_;
        static std::string    customPath_;
        static std::string    customArguments_;
    };
}
