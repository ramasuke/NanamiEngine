#pragma once
#include <memory>
#include <optional>

#include "../MainWindowBase.h"
#include "../Factory/MainWindowFactory.h"
#include "../Preview/ModelPreviewStage.h"
#include "../../../../Object/Field/Field.h"
#include "../../../../../Module/Asset/MV1/MV1File.h"
#include "AnimationPreviewSlot.h"

namespace NanamiEngine::Core::MainWindow
{
    /**
     * @brief モデルにアニメーションクリップを当てて再生・シーク・2 クリップのブレンドを確認するビューア
     *
     * @details
     *  表示は ModelViewWindow と同じ ModelPreviewStage を使う。
     *  モデル / アニメ .mv1 は Project ウィンドウからのドラッグ&ドロップ、または ModelViewWindow の「Open in AnimationView」で指定する。
     */
    class AnimationViewWindow final : public MainWindowBase<Module::Asset::Mv1File>
    {
    public:
        AnimationViewWindow();
        void AddContent(const std::shared_ptr<Module::Asset::Mv1File>& content) override;

    private:
        void OnUpdate () override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnSave   () override;

        /** @brief modelFile_ がドラッグ&ドロップ等で差し替わっていたらプレビューに反映する */
        void SyncModelField();
        /** @brief ルートフレームの水平移動を描画オフセットで打ち消し、モデルをその場に留める */
        void UpdateRootMotionLock(int modelHandle);
        void DrawTransportGui();
        void DrawRootMotionGui(int modelHandle);

        ModelPreviewStage             stage_;
        FIELD(Module::Asset::Mv1File) modelFile_;
        std::optional<Guid>           appliedModelGuid_;

        AnimationPreviewSlot slotA_;
        AnimationPreviewSlot slotB_;
        bool  isPlaying_   = true;
        bool  useBlend_    = false;
        /** @brief スロット B のブレンド率。A は 1 - blendWeight_ */
        float blendWeight_ = 0.5f;
        // ゲーム側 AnimationClipNode は NameCheck 無しでアタッチしている
        bool  nameCheck_   = false;

        bool                lockRootMotion_  = false;
        int                 rootFrameIndex_  = -1;
        std::optional<Guid> rootFrameModelGuid_;
    };

    REGISTER_MAIN_WINDOW(AnimationViewWindow)
}
