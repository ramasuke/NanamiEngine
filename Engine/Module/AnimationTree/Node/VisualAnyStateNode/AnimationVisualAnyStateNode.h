#pragma once
#include "../IAnimationNode.h"
#include "../LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::AnimationTree
{
    
    
    class AnimationVisualAnyStateNode final : public IAnimationNode
    {
    public:
        void           InitForGamePlay  (int   modelHandle ) override;
        void           OnUpdateAnimation(int   modelHandle, float timeScale) override;
        void           OnExitNode       (int   modelHandle ) override;
        void           OnUpdateBlendRate(float blendRate   ) override;
        
        [[nodiscard]] const Guid& GetGuid                 () const override { return guid_;     }
        [[nodiscard]] glm::vec2   Position                () const override { return position_; }
        void                      SetPosition(const glm::vec2& position) override { position_ = position; }
        [[nodiscard]] std::string GraphNodeName           () const override { return "Any State"; }
        [[nodiscard]] std::string GraphNodeDetail         () const override { return "どこからでも遷移"; }
        [[nodiscard]] float       GetAnimDuration_secs    () const override { return 0;         }
        R4::Observable<UpdateCallbackContext> OnUpdated() override { return R4::Observable<UpdateCallbackContext>::Never();            }

    private:
        glm::vec2         position_ = glm::vec2(0, 0);
        Guid              guid_;
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
