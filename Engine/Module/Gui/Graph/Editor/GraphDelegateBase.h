#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <unordered_set>

#include "GraphEditor.h"
#include "../../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::Gui::Graph
{
    class GraphEditorHost;

    /**
     * @brief GraphEditorHost に載せるグラフエディタの共通部分（AnimationTree / BehaviourTree）。
     * @note  選択（Guid で保持）、右クリックメニューの遅延オープン、Delete キー、CustomDraw 用の座標計算を持つ。
     *        派生クラスは Rebuild でデータモデルからノード列・リンク列を作り、各メニューの中身を描く。
     */
    class NANAMI_API GraphDelegateBase : public GraphEditor::Delegate
    {
    public:
        void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override;
        void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override;
        void RightClickLink(GraphEditor::LinkIndex linkIndex) override;

    protected:
        /**
         * @brief Rebuild → GraphEditor::Show → 右クリックメニュー → Delete キー の順に 1 フレーム描画する
         * @param readOnly 実行中ツリーの表示用。選択と Inspector 表示のみ行い、編集はしない
         */
        void DrawFrame(GraphEditorHost& host, bool readOnly);

        /** @brief データモデルからノード列・リンク列を作り直す（毎フレーム Show の前に呼ばれる） */
        virtual void Rebuild() = 0;
        [[nodiscard]] virtual Guid NodeGuid(GraphEditor::NodeIndex nodeIndex) const = 0;
        [[nodiscard]] virtual std::weak_ptr<Object::IObject> InspectTarget(GraphEditor::NodeIndex nodeIndex) const = 0;

        /** @brief 1 つの入力スロットに複数のリンクを繋げるか（false なら繋ぐ前に既存リンクを DelLink する） */
        [[nodiscard]] virtual bool AllowMultipleInputLinks() const { return false; }

        virtual void OnNodeSelected(GraphEditor::NodeIndex) {}
        virtual void OnRightClickNode(GraphEditor::NodeIndex) {}
        virtual void OnRightClickLink(GraphEditor::LinkIndex) {}
        virtual void DrawBackgroundMenu() {}
        virtual void DrawNodeMenu() {}
        virtual void DrawLinkMenu() {}
        virtual void DeleteSelection() {}

        [[nodiscard]] bool IsSelected(GraphEditor::NodeIndex nodeIndex) const;
        void ClearNodeSelection() { selectedNodes_.clear(); }
        void DrawFitAllMenuItem() const;

        /** @brief CustomDraw の本文矩形から拡大率を逆算する（CustomDraw には拡大率が渡らないため） */
        [[nodiscard]] float ZoomOf(const ImRect& body, float nodeWidth) const;
        /** @brief CustomDraw の本文矩形から、ヘッダーを含むノード全体の矩形を求める */
        [[nodiscard]] ImRect NodeFrame(const ImRect& body, float zoom) const;

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
