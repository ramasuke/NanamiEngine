#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Gui/Graph/Editor/GraphDelegateBase.h"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationTree;
    class AnimationNodePath;
    class IAnimationNode;
    
    class NANAMI_API AnimationTreeGraphDelegate final : public Gui::Graph::GraphDelegateBase
    {
    public:
        void Draw(AnimationTree& tree, Gui::Graph::GraphEditorHost& host, bool readOnly);

        // GraphEditor::Delegate
        bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override;
        void MoveSelectedNodes(ImVec2 delta) override;
        void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex,
                     GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override;
        void DelLink(GraphEditor::LinkIndex linkIndex) override;
        void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override;
        const size_t GetTemplateCount() override;
        const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override;
        const size_t GetNodeCount() override;
        const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override;
        const size_t GetLinkCount() override;
        const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override;
        void LinkClicked(GraphEditor::LinkIndex linkIndex) override;
        ImU32 LinkColor(GraphEditor::LinkIndex linkIndex, ImU32 defaultColor) override;

    protected:
        // GraphDelegateBase
        void Rebuild() override;
        [[nodiscard]] Guid NodeGuid(GraphEditor::NodeIndex nodeIndex) const override;
        [[nodiscard]] std::weak_ptr<Object::IObject> InspectTarget(GraphEditor::NodeIndex nodeIndex) const override;
        // NOTE: 1 つのステートに複数の遷移が入るので、既存の遷移を消さずに追加する
        [[nodiscard]] bool AllowMultipleInputLinks() const override { return true; }
        void OnNodeSelected(GraphEditor::NodeIndex nodeIndex) override;
        void OnRightClickNode(GraphEditor::NodeIndex nodeIndex) override;
        void OnRightClickLink(GraphEditor::LinkIndex linkIndex) override;
        void DrawBackgroundMenu() override;
        void DrawNodeMenu() override;
        void DrawLinkMenu() override;
        void DeleteSelection() override;

    private:
        enum TemplateKind : GraphEditor::TemplateIndex
        {
            TEMPLATE_ENTRY,
            TEMPLATE_ANY_STATE,
            TEMPLATE_CLIP,
            TEMPLATE_COUNT
        };

        struct NANAMI_API LinkEntry
        {
            std::shared_ptr<AnimationNodePath> path;
            GraphEditor::NodeIndex             from;
            GraphEditor::NodeIndex             to;
            bool                               isFromAnyState;
        };

        void TryAddLinkEntry(const std::shared_ptr<AnimationNodePath>& path, bool isFromAnyState);
        void DeleteNode(const std::shared_ptr<IAnimationNode>& node);
        void DeletePath(const std::shared_ptr<AnimationNodePath>& path);

        [[nodiscard]] GraphEditor::NodeIndex IndexOf(const std::shared_ptr<IAnimationNode>& node) const;
        [[nodiscard]] TemplateKind KindOf(GraphEditor::NodeIndex nodeIndex) const;

        AnimationTree* tree_ = nullptr;

        std::vector<std::shared_ptr<IAnimationNode>>             nodes_;
        std::vector<std::string>                                 names_;
        std::unordered_map<const IAnimationNode*, GraphEditor::NodeIndex> indexOf_;
        std::vector<LinkEntry>                                   links_;

        std::weak_ptr<AnimationNodePath> selectedPath_;

        std::weak_ptr<IAnimationNode>    menuNode_;
        std::weak_ptr<AnimationNodePath> menuPath_;
    };
}
