#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>

#include "../IEditorToolbarWidget.h"
#include "../../../../Process/AsyncProcess.h"

namespace NanamiEngine::Core::Toolbar
{
    /** @brief ゲーム DLL のビルドと差し替え (docs/HotReload.md §5)。DLL 構成のエディタでだけ出る */
    class NANAMI_API HotReloadToolbarWidget final : public IEditorToolbarWidget
    {
    public:
        [[nodiscard]] bool IsVisible() const override;
        void OnDraw(EditorToolbarWidgetContext& context) override;

    private:
        void BeginBuild();
        void PollBuild();
        void LogBuildErrors() const;

        Application::Process::AsyncProcess process_;
        std::filesystem::path              logPath_;
        std::filesystem::path              errorLogPath_;
        bool                               buildPending_ = false;
    };
}
