#pragma once
#include "../IAnimationNode.h"
#include "../../../Gui/Graph/NodeOption/NodeOption.h"

namespace NanamiEngine::Module::AnimationTree
{
    
    
    class AnimationVisualAnyStateNode final : public IAnimationNode
    {
    public:
        void           InitForGamePlay  (int   modelHandle ) override;
        void           OnUpdateAnimation(int   modelHandle ) override;
        void           OnExitNode       (int   modelHandle ) override;
        void           OnUpdateBlendRate(float blendRate   ) override;
        
        Gui::Graph::NodeDrawResult OnDrawGraphEditorGui(const ImVec2& offset, 
                                                        ImDrawList* drawList, 
                                                        std::weak_ptr<IAnimationNode> ownPtr) override;
        
        [[nodiscard]] const Guid& GetGuid                 () const override { return guid_;     }
        [[nodiscard]] glm::vec2   Position                () const override { return position_; }
        [[nodiscard]] float       GetAnimDuration_secs    () const override { return 0;         }
        rxcpp::observable<UpdateCallbackContext> OnUpdated() override { return rxcpp::subjects::subject<UpdateCallbackContext>().get_observable();            }

    private:
        glm::vec2         position_ = glm::vec2(0, 0);
        Guid              guid_;

        inline static const auto NODE_NAME = "AnimationVisualAnyStateNode";
        inline static const auto NODE_VISUAL_STYLE = Gui::Graph::NodeVisualStyle
        (
            IM_COL32(0  , 128, 255, 255),
            IM_COL32(200, 200, 200, 255),
            IM_COL32(180, 180, 100, 255),
            IM_COL32_WHITE
        );
#pragma region Serialization Function
    public:
        void OnDrawGui() {
            LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
        }
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<IAnimationNode>(this));
            archive(CEREAL_NVP(position_));
            archive(CEREAL_NVP(guid_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<IAnimationNode>(this));
            if (version >= 0) archive(CEREAL_NVP(position_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
        }
#pragma endregion
    };
};

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::AnimationTree::IAnimationNode, NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode);
#pragma endregion
