#pragma once
#include <memory>
#include <optional>

#include "../MainWindowBase.h"
#include "../Factory/MainWindowFactory.h"
#include "../../../Editor/Camera/Free/Editor3DCamera.h"
#include "../../../../../Module/Asset/MV1/MV1File.h"

namespace NanamiEngine::Scene
{
    class SceneGameObject;
}

namespace NanamiEngine::Module::Component
{
    class ModelRenderer;
}

namespace NanamiEngine::Core::MainWindow
{
    /**
     * @brief .mv1 をダブルクリックで開き、ModelRenderer に設定した GameObject として表示するビューア
     *
     * @details
     *  PrefabViewWindow と同じくバックバッファへ 3D 描画し、ImGui の "ModelView" パネルを上に重ねる。
     *  プレビュー用 GameObject は 1 つだけ持ち、一覧で選択したモデルを ModelRenderer::SetMv1File で差し替える。
     */
    class ModelViewWindow final : public MainWindowBase<Module::Asset::Mv1File>
    {
    public:
        ModelViewWindow();
        void AddContent(const std::shared_ptr<Module::Asset::Mv1File>& content) override;
        /** @brief 開いているモデルのうち guid のものを表示対象にする */
        void Select(const Guid& guid);

    private:
        void OnUpdate () override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnSave   () override;

        /** @brief プレビュー用 GameObject + ModelRenderer を遅延生成する(カレント MainWindow が自分であることを保証する) */
        void EnsurePreviewObject();
        /** @brief selectedGuid_ のモデルを ModelRenderer に設定する */
        void ApplySelectedModel();
        void CloseContent(const Guid& guid);
        /** @brief モデルのワールド AABB からカメラを正面に配置する */
        void FrameCamera();
        void DrawGrid() const;

        Module::Component::Editor3DCamera               camera_;
        std::shared_ptr<Scene::SceneGameObject>         previewObject_;
        std::weak_ptr<Module::Component::ModelRenderer> modelRenderer_;
        // Guid は既定コンストラクタで新規発行されるため「未選択」は optional で表す
        std::optional<Guid>                             selectedGuid_;
        // ハンドル取得後に一度だけ FrameCamera する(非同期ロード完了待ち)
        bool  pendingFrame_ = false;
        bool  showGrid_     = true;
        float gridStep_     = 10.0f;
    };

    REGISTER_MAIN_WINDOW(ModelViewWindow)
}
