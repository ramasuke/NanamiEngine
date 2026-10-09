#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../ApplicationBase.h"

namespace NanamiEngine::Core::Application
{
    class NANAMI_API EditorApplication final : public ApplicationBase
    {
    public:
        EditorApplication();
        static FileSystem::EditorDraggingHand& FileDraggingHand();

    private:
        void OnFrame  () override;
        void OnExit   () override;
        void OnDrawGui();

        // NOTE: 選択中 GameObject の Transform ギズモ。全ウィンドウ描画後に 1 フレーム 1 回呼ぶ
        void OnDrawGizmo();

        int   gizmoOperation_     = 7;     // ImGuizmo::TRANSLATE (TRANSLATE_X | _Y | _Z)
        int   gizmoMode_          = 0;     // ImGuizmo::LOCAL
        float gizmoSnapTranslate_ = 0.5f;
        float gizmoSnapRotate_    = 15.0f;
        float gizmoSnapScale_     = 0.1f;
    };
}
