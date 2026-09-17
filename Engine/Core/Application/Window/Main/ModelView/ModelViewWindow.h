#pragma once
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
    /**
     * @brief .mv1 をダブルクリックで開き、ModelRenderer に設定した GameObject として表示するビューア
     *
     * @details
     *  PrefabViewWindow と同じくバックバッファへ 3D 描画し、ImGui の "ModelView" パネルを上に重ねる。
     *  プレビュー用 GameObject は ModelPreviewStage が 1 つだけ持ち、一覧で選択したモデルを差し替える。
     */
    class ModelViewWindow final : public MainWindowBase<Module::Asset::Mv1File>
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        ModelViewWindow();
        void AddContent(const std::shared_ptr<Module::Asset::Mv1File>& content) override;
        /** @brief 開いているモデルのうち guid のものを表示対象にする */
        void Select(const Guid& guid);

    private:
        void OnUpdate () override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnSave   () override;

        void CloseContent(const Guid& guid);
        void OpenInAnimationView(const std::shared_ptr<Module::Asset::Mv1File>& model) const;

        ModelPreviewStage   stage_;
        // Guid は既定コンストラクタで新規発行されるため「未選択」は optional で表す
        std::optional<Guid> selectedGuid_;
    };

    REGISTER_MAIN_WINDOW(ModelViewWindow)
}
