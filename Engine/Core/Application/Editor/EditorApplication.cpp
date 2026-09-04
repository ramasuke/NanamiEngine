#include "EditorApplication.h"

#include <DxLib.h>
#include <memory>

#include "../../../../Libs/LibCore/ImGui/Wrapper/ImGuiWrapper.h"
#include "../../../../Libs/ImGui/ImGuiHelper.h"
#include "ImGuizmo.h"
#include "gtc/type_ptr.hpp"
#include "../Window/Toolbar/EditorToolbarWindow.h"
#include "../Window/Popup/Inspector/InspectorWindow.h"
#include "../../../Module/GameObject/Transform/Transform.h"
#include "EffekseerForDXLib.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "../LifeCycle/ApplicationLifeCycle.h"
#include "../Window/Popup/Group/PopupWindowGroup.h"

namespace
{
    // DxLib の MATRIX（行優先・行ベクトル規約、平行移動は 4 行目）を
    // glm::mat4（列優先・列ベクトル規約、平行移動は 4 列目）へ変換する。
    // 格納形式の読み替えは論理的な転置になり、ちょうど glm 規約の行列が得られる。
    // （LibCore::Glm::FromDxLibMatrix は論理行列を保持する変換なので、この用途では使えない）
    glm::mat4 DxMatrixToGlm(const MATRIX& m)
    {
        glm::mat4 result(1.0f);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result[i][j] = m.m[i][j];
        return result;
    }
}

Core::Application::EditorApplication::EditorApplication()
{
    SetDrawScreen   (DX_SCREEN_BACK);
    SetMouseDispFlag(true          );

    ImGuiWrapper::CreateInstance();
}

void Core::Application::EditorApplication::Run()
{
    while (ProcessMessage() >= 0)
    {
        ClearDrawScreen();
        
        ApplicationBase::Run();
        ImGuiWrapper::Instance().Update();
        ImGuizmo::BeginFrame();          // ImGui::NewFrame() 直後・フレーム1回だけ
        ApplicationLifeCycle_().OnUpdate();
        GetMainWindow()->OnUpdate();
        
        OnDrawGui();
        ImGui::EndFrame();

        RenderVertex();
        ImGuiWrapper::Instance().Draw();
        ScreenFlip();
    }
}

void Core::Application::EditorApplication::OnExit()
{
}

void Core::Application::EditorApplication::OnDrawGui()
{
    EditorToolbarWindow::OnDraw(PopupWindows());
    GetMainWindow()->OnDrawGui(MainWindow::MainWindowDrawGuiContext(FileDraggingHand()));
    PopupWindows_().OnDraw(FileDraggingHand());
    OnDrawGizmo();
}

void Core::Application::EditorApplication::OnDrawGizmo()
{
    // 選択中の GameObject を解決する。Inspector が複数あるときは最後に選択されたものを優先。
    std::shared_ptr<Module::GameObject::IGameObject> target;
    int bestOrder = -1;
    for (auto* inspector : PopupWindows().Catch<PopupWindow::InspectorWindow>())
    {
        if (inspector == nullptr || inspector->SelectionOrder() <= bestOrder)
            continue;

        auto object = std::dynamic_pointer_cast<Module::GameObject::IGameObject>(inspector->DisplayObject().lock());
        if (object)
        {
            target    = object;
            bestOrder = inspector->SelectionOrder();
        }
    }
    if (!target)
        return;

    const ImGuiIO& io = ImGui::GetIO();

    // 右ドラッグ（フリーカメラ操作）中・テキスト入力中はツール切替を受け付けない
    const bool cameraControlling = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if (!cameraControlling && !io.WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoOperation_ = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoOperation_ = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation_ = ImGuizmo::SCALE;
        if (ImGui::IsKeyPressed(ImGuiKey_X)) gizmoMode_ = (gizmoMode_ == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    }
    const auto operation = static_cast<ImGuizmo::OPERATION>(gizmoOperation_);
    const auto mode      = static_cast<ImGuizmo::MODE>(gizmoMode_);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

    // その時 DxLib が実際に使ったカメラ行列（編集=Editor3DCamera / プレイ=Cinemachine）を使う。
    // GameObject の world 行列は glm 規約なので、カメラ行列も glm 規約へ揃える。
    glm::mat4 view  = DxMatrixToGlm(GetCameraViewMatrix());
    glm::mat4 proj  = DxMatrixToGlm(GetCameraProjectionMatrix());
    glm::mat4 world = target->Transform().GetWorldMatrix();

    // Ctrl 押下中はスナップ
    const bool  useSnap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    float       snapValues[3] = {};
    const float* snap = nullptr;
    if (useSnap)
    {
        const float step = (operation == ImGuizmo::ROTATE) ? gizmoSnapRotate_
                         : (operation == ImGuizmo::SCALE)  ? gizmoSnapScale_
                         :                                   gizmoSnapTranslate_;
        snapValues[0] = snapValues[1] = snapValues[2] = step;
        snap = snapValues;
    }

    if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                             operation, mode, glm::value_ptr(world), nullptr, snap)
        && ImGuizmo::IsUsing())
    {
        target->Transform().SetWorldMatrix(world);
    }
}

Core::FileSystem::EditorDraggingHand& Core::Application::EditorApplication::FileDraggingHand()
{
    static FileSystem::EditorDraggingHand fileDraggingHand;
    return fileDraggingHand;
}