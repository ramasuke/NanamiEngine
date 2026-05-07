#pragma once
#include "../../../Core/Object/Field/Field.h"
#include "../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../Node/IAnimationNode.h"
#include "AdditionConditionGroup/AnimationNodePathAdditionConditionGroup.h"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationNodePath final : public Object::IObject
    {
    public:
        void InitNodePath(const std::shared_ptr<BlackBoard::ParameterGroup>& additionParams,
                          const std::function<std::weak_ptr<IAnimationNode>(const Guid&)>& findNode,
                          const std::function<void(const std::shared_ptr<IAnimationNode>&)>& onAddCurrentNode,
                          const std::function<void(const std::shared_ptr<IAnimationNode>&)>& onRemoveCurrentNode,
                          const std::function<void(AnimationNodePath*)>& onAddNextCurrentNodePath);
        void OnUpdateNodeAnimationBlend();
        ///TODO: 初期化時に設定するようにした方が良い(カプセル化)
        void SetFromNode  (const std::shared_ptr<IAnimationNode>& node);
        void SetFromNodeForGraphEditorGui(const std::shared_ptr<IAnimationNode>& visualNode, const std::shared_ptr<IAnimationNode>& node);
        ///TODO: 初期化時に設定するようにした方が良い(カプセル化) 
        void SetTargetNode(const std::shared_ptr<IAnimationNode>& node);
        void RemoveCurrentNodePath();

        std::shared_ptr<IAnimationNode> GetFromNode() const { return fromNode_.lock(); }
        std::shared_ptr<IAnimationNode> GetTargetNode() const { return nextNode_.lock(); }
        [[nodiscard]] glm::vec2   GetVisualFromNodePos  () const { return visualFromNode_.lock()->Position(); }
        [[nodiscard]] glm::vec2   GetVisualTargetNodePos() const { return nextNode_      .lock()->Position(); }
        [[nodiscard]] const Guid& GetGuid()                const override;
        

    private:
        void SubscribeUpdateNodeAnimationCallback();
        void TryAddNextCurrentNodePath (IAnimationNode::UpdateCallbackContext);

        std::shared_ptr<BlackBoard::ParameterGroup> additionParams_;
        std::weak_ptr<IAnimationNode> fromNode_;
        std::weak_ptr<IAnimationNode> nextNode_;
        std::weak_ptr<IAnimationNode> visualFromNode_;
        std::function<void(const std::shared_ptr<IAnimationNode>&)> onAddCurrentNode_;
        std::function<void(const std::shared_ptr<IAnimationNode>&)> onRemoveCurrentNode_;
        std::function<void(AnimationNodePath*)> onAddNextCurrentNodePath_;
        
        bool  isFirstBlendingAnimation_ = true;
        bool  isBlending_               = false;
        float transitionDuring_secs_    = 0;
        rxcpp::composite_subscription fromNodeSubscription_;
        rxcpp::subjects::subject<IAnimationNode::UpdateCallbackContext> onUpdated_ = rxcpp::subjects::subject<IAnimationNode::UpdateCallbackContext>();

        [[serialize(0)]] std::unique_ptr<AnimationNodePathAdditionConditionGroup> additionConditionGroup_ = std::make_unique<AnimationNodePathAdditionConditionGroup>();
        [[serialize(0)]] float transitionDuration_secs_ = 0;
        [[serialize(0)]] Guid fromNodeGuid_;
        [[serialize(0)]] Guid nextNodeGuid_;
        [[serialize(1)]] Guid visualFromNodeGuid_;
#pragma region Serialization Function
public:
void OnDrawGui() override
{
    LibCore::ImGuiHelper::OnDrawInputField("additionConditionGroup_", additionConditionGroup_);
    LibCore::ImGuiHelper::OnDrawInputField("transitionDuration_secs_", transitionDuration_secs_);
    LibCore::ImGuiHelper::OnDrawInputField("fromNodeGuid_", fromNodeGuid_);
    LibCore::ImGuiHelper::OnDrawInputField("nextNodeGuid_", nextNodeGuid_);
    LibCore::ImGuiHelper::OnDrawInputField("visualFromNodeGuid_", visualFromNodeGuid_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<Object::IObject>(this));
    archive(CEREAL_NVP(additionConditionGroup_));
    archive(CEREAL_NVP(transitionDuration_secs_));
    archive(CEREAL_NVP(fromNodeGuid_));
    archive(CEREAL_NVP(nextNodeGuid_));
    archive(CEREAL_NVP(visualFromNodeGuid_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<Object::IObject>(this));
    if (version >= 0) archive(CEREAL_NVP(additionConditionGroup_));
    if (version >= 0) archive(CEREAL_NVP(transitionDuration_secs_));
    if (version >= 0) archive(CEREAL_NVP(fromNodeGuid_));
    if (version >= 0) archive(CEREAL_NVP(nextNodeGuid_));
    if (version >= 1) archive(CEREAL_NVP(visualFromNodeGuid_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationNodePath, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePath);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, NanamiEngine::Module::AnimationTree::AnimationNodePath);
#pragma endregion