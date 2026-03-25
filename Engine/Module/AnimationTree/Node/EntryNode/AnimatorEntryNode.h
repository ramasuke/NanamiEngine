#pragma once
#include "vec2.hpp"
#include "../../../Asset/MV1/MV1File.h"
#include "../../../../Core/Object/Field/Field.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../IAnimationNode.h"
#include "../../../Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"

namespace NanamiEngine::Module::AnimationTree
{
    constexpr auto ENTRY_NODE_SIZE = glm::vec2(120.0f, 60.0f);
    
    class AnimatorEntryNode final : public IAnimationNode
    {
    public:
        void InitForGamePlay(int modelHandle) override;
        [[nodiscard]] const Guid& GetGuid()  const override { return guid_;         }
        [[nodiscard]] glm::vec2   Position() const override { return position_;     }
        [[nodiscard]] float GetAnimDuration_secs() const override { return 0; }
        void OnUpdateBlendRate(float  blendRate) override;
        void OnUpdateAnimation(int  modelHandle) override;
        rxcpp::observable<UpdateCallbackContext> OnUpdated() override { return onUpdate_.get_observable(); }
        Gui::Graph::NodeDrawResult OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList, std::weak_ptr<IAnimationNode> ownPtr) override;
        void OnExitNode(int modelHandle) override;
        
    private:
        glm::vec2 position_;
        Guid guid_;
        float speed_ = 1.0f;
        rxcpp::subjects::subject<UpdateCallbackContext> onUpdate_ = rxcpp::subjects::subject<UpdateCallbackContext>();
        int animationModelHandle_ = -1;
        bool isDrag_ = false;

        inline static const auto NODE_NAME = "AnimatorEntryNode";
        inline static const auto NODE_VISUAL_STYLE = Gui::Graph::NodeVisualStyle
        (
            IM_COL32(0, 200, 0, 255),
            IM_COL32(200, 200, 200, 255),
            IM_COL32(180, 180, 100, 255),
            IM_COL32_WHITE
        );
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
    LibCore::ImGuiHelper::OnDrawInputField("speed_", speed_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<IAnimationNode>(this));
    archive(CEREAL_NVP(position_));
    archive(CEREAL_NVP(guid_));
    archive(CEREAL_NVP(speed_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<IAnimationNode>(this));
    if (version >= 0) archive(CEREAL_NVP(position_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
    if (version >= 0) archive(CEREAL_NVP(speed_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimatorEntryNode, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimatorEntryNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::AnimationTree::IAnimationNode, NanamiEngine::Module::AnimationTree::AnimatorEntryNode);
#pragma endregion
