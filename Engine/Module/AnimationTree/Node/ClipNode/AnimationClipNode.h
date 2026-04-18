#pragma once
#include "vec2.hpp"
#include "../../../Asset/MV1/MV1File.h"
#include "../../../../Core/Object/Field/Field.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../IAnimationNode.h"
#include "../../../Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationClipNode final : public IAnimationNode
    {
    public:
        explicit AnimationClipNode(glm::vec2 position = glm::vec2(0, 0));
        void InitForGamePlay  (int   modelHandle) override;
        void OnUpdateAnimation(int   modelHandle) override;
        void OnExitNode       (int   modelHandle) override;
        void OnUpdateBlendRate(float blendRate  ) override;

        Gui::Graph::NodeDrawResult OnDrawGraphEditorGui(const ImVec2& offset,
                                                        ImDrawList* drawList,
                                                        std::weak_ptr<IAnimationNode> ownPtr) override;
        
        [[nodiscard]] const Guid& GetGuid             () const override { return guid_;                                       }
        [[nodiscard]] glm::vec2   Position            () const override { return position_;                                   }
        [[nodiscard]] float       GetAnimDuration_secs() const override { return duration_secs_ - blendAnimationOffset_secs_; }
        rxcpp::observable<UpdateCallbackContext> OnUpdated() override;

    private:
        FIELD(Asset::Mv1File) animationFile_;
        std::string           name_ = "ClipNode";
        glm::vec2             position_;
        Guid                  guid_;
        float                 speed_                  = 1.0f;
        float                 blendAnimationOffset_secs_   = 0.0f;
        
        float                 blendRate_              = 1.0f;
        int                   attachedAnimationIndex_ = -1;
        int                   dxlibAnimationIndex_    = -1; 
        float                 duration_secs_          = 0;
        float                 during_secs_            = 0;
        int                   modelAnimationIndex_    = 0;
        rxcpp::subjects::subject<UpdateCallbackContext> onUpdate_ = rxcpp::subjects::subject<UpdateCallbackContext>();
        
        inline static const auto NODE_VISUAL_STYLE = Gui::Graph::NodeVisualStyle
        (
            IM_COL32(50 , 50 , 70 , 255),
            IM_COL32(200, 200, 200, 255),
            IM_COL32(180, 180, 100, 255),
            IM_COL32_WHITE
        );
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<IAnimationNode>(this));
    archive(CEREAL_NVP(animationFile_));
    archive(CEREAL_NVP(name_));
    archive(CEREAL_NVP(position_));
    archive(CEREAL_NVP(guid_));
    archive(CEREAL_NVP(speed_));
    archive(CEREAL_NVP(blendAnimationOffset_secs_));
    archive(CEREAL_NVP(modelAnimationIndex_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<IAnimationNode>(this));
    if (version >= 0) archive(CEREAL_NVP(animationFile_));
    if (version >= 0) archive(CEREAL_NVP(name_));
    if (version >= 0) archive(CEREAL_NVP(position_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
    if (version >= 0) archive(CEREAL_NVP(speed_));
    if (version >= 1) archive(CEREAL_NVP(blendAnimationOffset_secs_));
    if (version >= 2) archive(CEREAL_NVP(modelAnimationIndex_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationClipNode, 2);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationClipNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::AnimationTree::IAnimationNode, NanamiEngine::Module::AnimationTree::AnimationClipNode);
#pragma endregion
