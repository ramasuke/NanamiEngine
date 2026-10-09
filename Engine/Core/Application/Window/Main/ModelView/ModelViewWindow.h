#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <optional>

#include "../MainWindowBase.h"
#include "../Factory/MainWindowFactory.h"
#include "../Preview/ModelPreviewStage.h"
#include "../../../../../Module/Asset/MV1/MV1File.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Core::MainWindow
{
    // NOTE: .mv1 を ModelRenderer 付きの GameObject として表示するビューア
    class NANAMI_API ModelViewWindow final : public MainWindowBase<Module::Asset::Mv1File>
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        ModelViewWindow();
        void AddContent(const std::shared_ptr<Module::Asset::Mv1File>& content) override;
        // NOTE: 開いているモデルのうち guid のものを表示対象にする
        void Select(const Guid& guid);

    private:
        void OnUpdate () override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnSave   () override;

        void CloseContent(const Guid& guid);
        void OpenInAnimationView(const std::shared_ptr<Module::Asset::Mv1File>& model) const;

        ModelPreviewStage   stage_;
        // NOTE: Guid は既定コンストラクタで新規発行されるため「未選択」は optional で表す
        std::optional<Guid> selectedGuid_;
    };

    REGISTER_MAIN_WINDOW(ModelViewWindow, "Viewer")
}
