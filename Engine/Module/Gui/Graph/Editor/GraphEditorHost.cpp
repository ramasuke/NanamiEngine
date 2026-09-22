#include "GraphEditorHost.h"

#include "imgui_internal.h"
#include "../LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../Core/Application/ApplicationBase.h"
#include "../../../../Core/Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"

namespace NanamiEngine::Module::Gui::Graph
{
    namespace
    {
        /** @brief これより小さいキャンバスでは Fit しない（ウィンドウ出現直後は 0 サイズで、ズーム率が 0 になるため） */
        constexpr float K_MIN_FIT_CANVAS_SIZE = 32.0f;
        constexpr ImU32 K_READ_ONLY_BACKGROUND = IM_COL32(30, 34, 40, 255);
        constexpr ImU32 K_EDIT_BACKGROUND      = IM_COL32(34, 34, 38, 255);
    }

    GraphEditorHost::GraphEditorHost()
    {
        options_.mBackgroundColor          = K_EDIT_BACKGROUND;
        options_.mGridColor                = IM_COL32(255, 255, 255, 10);
        options_.mGridColor2               = IM_COL32(255, 255, 255, 26);
        options_.mSelectedNodeBorderColor  = IM_COL32(255, 170, 50, 255);
        options_.mNodeBorderColor          = IM_COL32(0, 0, 0, 120);
        options_.mQuadSelection            = IM_COL32(80, 140, 255, 40);
        options_.mQuadSelectionBorder      = IM_COL32(80, 140, 255, 200);
        options_.mDefaultSlotColor         = IM_COL32(200, 200, 200, 255);
        options_.mFrameFocus               = IM_COL32(80, 140, 255, 160);
        options_.mLineThickness            = 2.5f;
        options_.mGridSize                 = 32.0f;
        options_.mRounding                 = 6.0f;
        options_.mBorderSelectionThickness = 4.0f;
        options_.mBorderThickness          = 2.0f;
        options_.mNodeSlotRadius           = 6.0f;
        options_.mMinZoom                  = 0.2f;
        options_.mMaxZoom                  = 1.6f;
        options_.mHeaderHeight             = 22.0f;
        options_.mDrawIONameOnHover        = true;
    }

    void GraphEditorHost::Draw(GraphEditor::Delegate& delegate, const bool readOnly)
    {
        DrawToolbar();

        isFocused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        if (isFocused_ && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false))
            RequestFit();

        options_.mReadOnly        = readOnly;
        options_.mBackgroundColor = readOnly ? K_READ_ONLY_BACKGROUND : K_EDIT_BACKGROUND;
        options_.mMinimap         = showMinimap_ ? ImRect(ImVec2(0.78f, 0.78f), ImVec2(0.99f, 0.99f))
                                                 : ImRect(ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f));

        canvasOrigin_ = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < 1.0f || canvasSize.y < 1.0f)
            return;

        GraphEditor::FitOnScreen fit = GraphEditor::Fit_None;
        if (fit_ != GraphEditor::Fit_None && canvasSize.x >= K_MIN_FIT_CANVAS_SIZE && canvasSize.y >= K_MIN_FIT_CANVAS_SIZE)
        {
            fit  = fit_;
            fit_ = GraphEditor::Fit_None;
        }

        GraphEditor::Show(delegate, options_, viewState_, true, &fit);

        // Fit は最大 1 倍までしか拡大しないが、最小ズームは下回り得るので揃える
        viewState_.mFactorTarget = ImClamp(viewState_.mFactorTarget, options_.mMinZoom, options_.mMaxZoom);
    }

    ImVec2 GraphEditorHost::ScreenToGraph(const ImVec2& screenPosition) const
    {
        return (screenPosition - canvasOrigin_) / viewState_.mFactor - viewState_.mPosition;
    }

    void GraphEditorHost::RequestFit(const bool selectedOnly)
    {
        fit_ = selectedOnly ? GraphEditor::Fit_SelectedNodes : GraphEditor::Fit_AllNodes;
    }

    void GraphEditorHost::DrawToolbar()
    {
        if (ImGui::SmallButton("Fit All (F)"))
            RequestFit();
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit Selected"))
            RequestFit(true);
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &options_.mRenderGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Minimap", &showMinimap_);
        ImGui::SameLine();
        ImGui::Checkbox("Curves", &options_.mDisplayLinksAsCurves);
        ImGui::SameLine();
        ImGui::TextDisabled("%3.0f%%", viewState_.mFactor * 100.0f);
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("中ボタンドラッグ: パン\n"
                              "ホイール: ズーム\n"
                              "左ドラッグ(空き領域): 範囲選択 / Shift: 追加選択\n"
                              "右クリック: メニュー\n"
                              "F: 全体表示");
        }
    }

    GraphEditor::Template MakeNodeTemplate(const ImU32 headerColor, const ImU8 inputCount, const ImU8 outputCount)
    {
        return GraphEditor::Template
        {
            headerColor,
            IM_COL32(48, 48, 54, 245),
            IM_COL32(62, 62, 70, 245),
            inputCount,
            nullptr,
            nullptr,
            outputCount,
            nullptr,
            nullptr
        };
    }

    void ShowInInspector(const std::weak_ptr<Object::IObject>& object)
    {
        if (object.expired())
            return;

        for (auto* inspector : Core::Application::ApplicationBase::PopupWindows().Catch<Core::PopupWindow::InspectorWindow>())
        {
            inspector->TryAddDisplayObject(object);
        }
    }
}
