#include "GraphDelegateBase.h"

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
    }

    void GraphDelegateBase::DrawFrame(GraphEditorHost& host, const bool readOnly)
    {
        host_     = &host;
        readOnly_ = readOnly;

        Rebuild();

        // 削除済みノードの選択を掃除する
        std::unordered_set<Guid, GuidHash> alive;
        for (GraphEditor::NodeIndex i = 0; i < GetNodeCount(); ++i)
            alive.insert(NodeGuid(i));
        std::erase_if(selectedNodes_, [&](const Guid& guid) { return !alive.contains(guid); });

        host.Options().mAllowMultipleInputLinks = AllowMultipleInputLinks();
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

    float GraphDelegateBase::ZoomOf(const ImRect& body, const float nodeWidth) const
    {
        // 本文 = ノード矩形 * 拡大率 から、拡大されない角丸ぶんを上下左右に削ったもの
        return (body.GetWidth() + host_->Options().mRounding * 2.0f) / nodeWidth;
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
