#include "GraphDelegateBase.h"

#include <cmath>
#include <vector>

#include "imgui_internal.h"
#include "GraphEditorHost.h"

namespace NanamiEngine::Module::Gui::Graph
{
    namespace
    {
        constexpr auto K_BACKGROUND_MENU = "##GraphBackgroundMenu";
        constexpr auto K_NODE_MENU       = "##GraphNodeMenu";
        constexpr auto K_LINK_MENU       = "##GraphLinkMenu";

        constexpr ImVec2 K_MIN_NODE_SIZE     = ImVec2(120.0f, 60.0f);
        constexpr float  K_TITLE_PADDING     = 6.0f;
        constexpr float  K_DETAIL_FONT_RATIO = 0.85f;
        constexpr float  K_DETAIL_MIN_ZOOM   = 0.6f;
        constexpr float  K_MIN_TEXT_SIZE     = 6.0f;
        constexpr auto   K_BADGE_SAMPLE      = " #00";
    }

    void GraphDelegateBase::DrawFrame(GraphEditorHost& host, const bool readOnly)
    {
        host_     = &host;
        readOnly_ = readOnly;

        Rebuild();

        // NOTE: 削除済みノードの選択を掃除する
        std::unordered_set<Guid, GuidHash> alive;
        for (GraphEditor::NodeIndex i = 0; i < GetNodeCount(); ++i)
            alive.insert(NodeGuid(i));
        std::erase_if(selectedNodes_, [&](const Guid& guid) { return !alive.contains(guid); });

        host.Options().mAllowMultipleInputLinks = AllowMultipleInputLinks();
        host.SetToolbarExtra([this] { DrawToolbarItems(); });
        host.Draw(*this, readOnly);
        DrawContextMenus();

        if (!readOnly_ && host.IsFocused() && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            DeleteSelection();
    }

    void GraphDelegateBase::SelectNode(const GraphEditor::NodeIndex nodeIndex, const bool selected)
    {
        if (nodeIndex >= GetNodeCount())
            return;

        if (!selected)
        {
            selectedNodes_.erase(NodeGuid(nodeIndex));
            return;
        }

        selectedNodes_.insert(NodeGuid(nodeIndex));
        OnNodeSelected(nodeIndex);
        ShowInInspector(InspectTarget(nodeIndex));
    }

    void GraphDelegateBase::RightClick(const GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex, GraphEditor::SlotIndex)
    {
        menuGraphPosition_ = host_->ScreenToGraph(ImGui::GetIO().MousePos);
        if (nodeIndex < GetNodeCount())
        {
            OnRightClickNode(nodeIndex);
            pendingMenu_ = PendingMenu::Node;
        }
        else
        {
            pendingMenu_ = PendingMenu::Background;
        }
    }

    void GraphDelegateBase::RightClickLink(const GraphEditor::LinkIndex linkIndex)
    {
        if (linkIndex >= GetLinkCount())
            return;

        menuGraphPosition_ = host_->ScreenToGraph(ImGui::GetIO().MousePos);
        OnRightClickLink(linkIndex);
        pendingMenu_ = PendingMenu::Link;
    }

    bool GraphDelegateBase::IsSelected(const GraphEditor::NodeIndex nodeIndex) const
    {
        return selectedNodes_.contains(NodeGuid(nodeIndex));
    }

    void GraphDelegateBase::DrawFitAllMenuItem() const
    {
        if (ImGui::MenuItem("Fit All", "F"))
            host_->RequestFit();
    }

    float GraphDelegateBase::Zoom() const
    {
        return host_->Zoom();
    }

    ImVec2 GraphDelegateBase::MeasureNodeSize(const std::string& title, const std::string& detail, const bool hasBadge) const
    {
        const auto& options        = host_->Options();
        ImFont*     font           = FontForSize(0.0f);
        const float fontSize       = font->FontSize;
        const float detailFontSize = fontSize * K_DETAIL_FONT_RATIO;

        float titleWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, title.c_str()).x + K_TITLE_PADDING * 2.0f;
        if (hasBadge)
            titleWidth += font->CalcTextSizeA(detailFontSize, FLT_MAX, 0.0f, K_BADGE_SAMPLE).x;

        ImVec2 detailText(0.0f, 0.0f);
        if (!detail.empty())
            detailText = font->CalcTextSizeA(detailFontSize, FLT_MAX, 0.0f, detail.c_str());
        const float detailWidth  = detailText.x + options.mRounding * 2.0f;
        const float detailHeight = detailText.y + options.mHeaderHeight + options.mRounding * 2.0f;

        return ImVec2(std::ceil(ImMax(K_MIN_NODE_SIZE.x, ImMax(titleWidth, detailWidth))),
                      std::ceil(ImMax(K_MIN_NODE_SIZE.y, detailHeight)));
    }

    float GraphDelegateBase::DetailFontSize() const
    {
        return FontForSize(0.0f)->FontSize * K_DETAIL_FONT_RATIO * Zoom();
    }

    bool GraphDelegateBase::DetailVisible() const
    {
        return Zoom() >= K_DETAIL_MIN_ZOOM && DetailFontSize() >= K_MIN_TEXT_SIZE;
    }

    void GraphDelegateBase::DrawNodeDetail(ImDrawList* drawList, const ImRect& body, const std::string& detail, const ImU32 color) const
    {
        const float fontSize = DetailFontSize();
        if (detail.empty() || !DetailVisible())
            return;

        drawList->PushClipRect(body.Min, body.Max, true);
        drawList->AddText(FontForSize(fontSize), fontSize, body.Min, color, detail.c_str());
        drawList->PopClipRect();
    }

    ImRect GraphDelegateBase::NodeFrame(const ImRect& body, const float zoom) const
    {
        const auto& options = host_->Options();
        return ImRect(body.Min - ImVec2(options.mRounding, options.mHeaderHeight * zoom + options.mRounding),
                      body.Max + ImVec2(options.mRounding, options.mRounding));
    }

    void GraphDelegateBase::DrawContextMenus()
    {
        switch (pendingMenu_)
        {
        case PendingMenu::Background: ImGui::OpenPopup(K_BACKGROUND_MENU); break;
        case PendingMenu::Node:       ImGui::OpenPopup(K_NODE_MENU);       break;
        case PendingMenu::Link:       ImGui::OpenPopup(K_LINK_MENU);       break;
        case PendingMenu::None:       break;
        }
        pendingMenu_ = PendingMenu::None;

        if (ImGui::BeginPopup(K_BACKGROUND_MENU))
        {
            DrawBackgroundMenu();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup(K_NODE_MENU))
        {
            DrawNodeMenu();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup(K_LINK_MENU))
        {
            DrawLinkMenu();
            ImGui::EndPopup();
        }
    }
}
