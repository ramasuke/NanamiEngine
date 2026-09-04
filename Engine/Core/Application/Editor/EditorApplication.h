#pragma once
#include "../ApplicationBase.h"

namespace NanamiEngine::Core::Application
{
    class EditorApplication final : public ApplicationBase
    {
    public:
        ///初期設定
        EditorApplication();
        static FileSystem::EditorDraggingHand& FileDraggingHand();

    private:
        void Run      () override;
        void OnExit   () override;
        void OnDrawGui();

        /// 選択中 GameObject に対する Transform ギズモ。1フレーム1回、全ウィンドウ描画後に呼ぶ。
        void OnDrawGizmo();

        int   gizmoOperation_     = 7;     // ImGuizmo::TRANSLATE (TRANSLATE_X | _Y | _Z)
        int   gizmoMode_          = 0;     // ImGuizmo::LOCAL
        float gizmoSnapTranslate_ = 0.5f;
        float gizmoSnapRotate_    = 15.0f;
        float gizmoSnapScale_     = 0.1f;
    };
}
