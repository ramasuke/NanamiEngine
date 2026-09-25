#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "../Interface/IMainWindow.h"
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

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Core::MainWindow
{
    /**
     * @brief ModelViewWindow / AnimationViewWindow 共通の、モデル 1 体を置いて眺めるためのプレビュー環境
     *
     * @details
     *  プレビュー用 GameObject + ModelRenderer とエディタカメラ・グリッドを持つ。
     *  モデルの描画自体は所有ウィンドウの LifeCycle().OnUpdateForEditor() で行われる。
     */
    class NANAMI_API ModelPreviewStage final
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        /** @brief owner をカレント MainWindow にしてから model をプレビュー GameObject の ModelRenderer に設定する */
        void SetModel(const std::shared_ptr<IMainWindow>& owner, const std::shared_ptr<Module::Asset::Mv1File>& model);
        /** @brief プレビュー用 GameObject ごと破棄する(ModelRenderer::OnDestroy で MV1DeleteModel される) */
        void ClearModel();
        /** @brief 非同期ロード中に設定して handle が -1 のままのモデルを、ロード完了後に取り直す */
        void PollModelLoad();
        /** @brief カメラ更新とグリッド描画。LifeCycle の描画より前に呼ぶ */
        void UpdateViewport();
        void RequestFrame() { pendingFrame_ = true; }

        void DrawViewportGui();
        void DrawPreviewObjectGui() const;

        /** @brief 表示中モデルの DxLib ハンドル。未設定・ロード待ちなら -1 */
        [[nodiscard]] int ModelHandle() const;
        [[nodiscard]] std::shared_ptr<Module::Component::ModelRenderer> Renderer() const { return modelRenderer_.lock(); }
        [[nodiscard]] glm::mat4 PreviewWorldMatrix() const;

    private:
        void EnsurePreviewObject(const std::shared_ptr<IMainWindow>& owner);
        /** @brief モデルのワールド AABB から、frameViewDirection_ の向きで全体が入る位置にカメラを置く */
        void FrameCamera();
        void DrawGrid() const;
        [[nodiscard]] static glm::vec3 DefaultFrameViewDirection();

        Module::Component::Editor3DCamera               camera_;
        glm::vec3                                       frameViewDirection_ = DefaultFrameViewDirection();
        std::shared_ptr<Scene::SceneGameObject>         previewObject_;
        std::weak_ptr<Module::Component::ModelRenderer> modelRenderer_;
        std::shared_ptr<Module::Asset::Mv1File>         model_;
        // ハンドル取得後に一度だけ FrameCamera する(非同期ロード完了待ち)
        bool  pendingFrame_ = false;
        bool  showGrid_     = true;
        float gridStep_     = 10.0f;
    };
}
