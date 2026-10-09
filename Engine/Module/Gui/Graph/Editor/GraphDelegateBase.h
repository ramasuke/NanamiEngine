#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <string>
#include <unordered_set>

#include "GraphEditor.h"
#include "../../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::Gui::Graph
{
    class GraphEditorHost;

    // NOTE: GraphEditorHost に載せるグラフエディタの共通部分
    // NOTE: 派生クラスは Rebuild でデータモデルからノード列・リンク列を作る
    class NANAMI_API GraphDelegateBase : public GraphEditor::Delegate
    {
    public:
        void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override;
        void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override;
        void RightClickLink(GraphEditor::LinkIndex linkIndex) override;

    protected:
        // NOTE: Rebuild → GraphEditor::Show → 右クリックメニュー → Delete キー の順に 1 フレーム描画する
        // NOTE: readOnly なら選択と Inspector 表示だけで編集しない
        void DrawFrame(GraphEditorHost& host, bool readOnly);

        // NOTE: データモデルからノード列・リンク列を作り直す（毎フレーム Show の前に呼ばれる）
        virtual void Rebuild() = 0;
        [[nodiscard]] virtual Guid NodeGuid(GraphEditor::NodeIndex nodeIndex) const = 0;
        [[nodiscard]] virtual std::weak_ptr<Object::IObject> InspectTarget(GraphEditor::NodeIndex nodeIndex) const = 0;

        // NOTE: 1 つの入力スロットに複数のリンクを繋げるか（false なら繋ぐ前に既存リンクを DelLink する）
        [[nodiscard]] virtual bool AllowMultipleInputLinks() const { return false; }

        virtual void OnNodeSelected(GraphEditor::NodeIndex) {}
        virtual void OnRightClickNode(GraphEditor::NodeIndex) {}
        virtual void OnRightClickLink(GraphEditor::LinkIndex) {}
        virtual void DrawBackgroundMenu() {}
        // NOTE: ツールバーの右端に足すもの（readOnly では呼ばれない）
        virtual void DrawToolbarItems() {}
        virtual void DrawNodeMenu() {}
        virtual void DrawLinkMenu() {}
        virtual void DeleteSelection() {}

        [[nodiscard]] bool IsSelected(GraphEditor::NodeIndex nodeIndex) const;
        void ClearNodeSelection() { selectedNodes_.clear(); }
        void DrawFitAllMenuItem() const;

        [[nodiscard]] float Zoom() const;
        // NOTE: CustomDraw の本文矩形から、ヘッダーを含むノード全体の矩形を求める
        [[nodiscard]] ImRect NodeFrame(const ImRect& body, float zoom) const;

        // NOTE: 見出しと本文が切れずに収まるノードの大きさ（拡大率 1）
        [[nodiscard]] ImVec2 MeasureNodeSize(const std::string& title, const std::string& detail, bool hasBadge) const;

        // NOTE: 本文を描く（遠景では描かない）
        void DrawNodeDetail(ImDrawList* drawList, const ImRect& body, const std::string& detail, ImU32 color) const;
        [[nodiscard]] float DetailFontSize() const;
        [[nodiscard]] bool DetailVisible() const;

        GraphEditorHost*                   host_              = nullptr;
        bool                               readOnly_          = false;
        ImVec2                             menuGraphPosition_ = ImVec2(0, 0);
        std::unordered_set<Guid, GuidHash> selectedNodes_;

    private:
        enum class PendingMenu
        {
            None,
            Background,
            Node,
            Link
        };

        void DrawContextMenus();

        PendingMenu pendingMenu_ = PendingMenu::None;
    };
}
