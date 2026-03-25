#pragma once
#include "../../Core/Object/IObject.h"
#include "../Asset/Factory/AssetFactory.h"
#include "../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Node/EntryNode/AnimatorEntryNode.h"
#include "Node/VisualAnyStateNode/AnimationVisualAnyStateNode.h"
#include "NodePath/AnimationNodePath.h"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationTree final : public Object::IObject
    {
    public:
        explicit AnimationTree(std::string filePath = "");
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        void OnSave();
        void OnUpdate(int modelHandle) const;
        void OnDrawGraphEditorGui();
        void OnDrawAllNodeGui     (ImDrawList* drawList, ImVec2 offset);
        void OnDrawDraggingNodeGui(ImDrawList* drawList, ImVec2 offset) const;
        void OnDrawGui() override;
        [[nodiscard]] BlackBoard::ParameterGroup& Param() const { return *additionConditionParameters_; }
        
        /** @warning Playモード時は呼び出し必須 */
        void InitForAnimator(int modelHandle);

    private:
        void AddCurrentNode    (const std::shared_ptr<IAnimationNode>& node);
        void AddCurrentNodePath(AnimationNodePath* nodePath, int modelHandle);
        void RemoveCurrentNode (const std::shared_ptr<IAnimationNode>& node, int modelHandle);

        [[nodiscard]] std::vector<std::shared_ptr<AnimationNodePath>> AllNodePaths() const;
        std::weak_ptr<IAnimationNode> FindNode(const Guid& guid);
        void CreateNode();
        
        Guid guid_;
        std::string filePath_;
        std::shared_ptr<AnimatorEntryNode> entryNode_ = std::make_shared<AnimatorEntryNode>();
        std::shared_ptr<AnimationVisualAnyStateNode> visualAnyStateNode_ = std::make_shared<AnimationVisualAnyStateNode>();
        std::unordered_map<Guid, std::shared_ptr<IAnimationNode>, GuidHash> nodes_;
        std::vector<std::shared_ptr<AnimationNodePath>> fromNodeNodePaths_;
        std::vector<std::shared_ptr<AnimationNodePath>> fromAnyStateNodeNodePaths_;
        std::vector<std::shared_ptr<IAnimationNode   >> currentNodes_;
        AnimationNodePath*                              currentNodePath_ = nullptr;
        std::shared_ptr<BlackBoard::ParameterGroup> additionConditionParameters_ = std::make_shared<BlackBoard::ParameterGroup>();
        
        std::weak_ptr<IAnimationNode> dragStartNode_;
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationTree, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationTree);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, NanamiEngine::Module::AnimationTree::AnimationTree);
#pragma endregion