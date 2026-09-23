#pragma once
#include "vec2.hpp"
#include "../../../Asset/MV1/MV1File.h"
#include "../../../../Core/Object/Field/Field.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../IAnimationNode.h"
#include "../LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationClipNode final : public IAnimationNode
    {
    public:
        explicit AnimationClipNode(glm::vec2 position = glm::vec2(0, 0));
        ~AnimationClipNode() override;
        AnimationClipNode(const AnimationClipNode&)            = delete;
        AnimationClipNode& operator=(const AnimationClipNode&) = delete;
        void InitForGamePlay  (int   modelHandle) override;
        void OnUpdateAnimation(int   modelHandle, float timeScale) override;
        void OnExitNode       (int   modelHandle) override;
        void OnUpdateBlendRate(float blendRate  ) override;

        [[nodiscard]] const Guid& GetGuid             () const override { return guid_;                                       }
        [[nodiscard]] glm::vec2   Position            () const override { return position_;                                   }
        void                      SetPosition(const glm::vec2& position) override { position_ = position;                        }
        [[nodiscard]] std::string GraphNodeName       () const override { return name_;                                       }
        [[nodiscard]] std::string GraphNodeDetail     () const override;
        [[nodiscard]] float       GetAnimDuration_secs() const override { return ClipEndTime() - blendAnimationOffset_secs_; }
        R4::Observable<UpdateCallbackContext> OnUpdated() override;

        [[nodiscard]] float GetDuringSecs() const { return during_secs_; }
        void                SetDuringSecs(float secs) { during_secs_ = secs; }
        [[nodiscard]] float GetBlendRate () const { return blendRate_;    }

        [[nodiscard]] const std::string& Name() const { return name_; }
        [[nodiscard]] ClipProgress GetClipProgress() const;

    private:
        void ReleaseAnimationModel();
        [[nodiscard]] float ClipEndTime() const;

        FIELD(Asset::Mv1File) animationFile_;
        std::string           name_ = "ClipNode";
        glm::vec2             position_;
        Guid                  guid_;
        float                 speed_                  = 1.0f;
        float                 blendAnimationOffset_secs_   = 0.0f;
        float                 clipStartTime_          = 0.0f;
        /** @brief 0 以下ならクリップ末尾 */
        float                 clipEndTime_            = 0.0f;
        bool                  isLoop_                 = true;

        float                 blendRate_              = 1.0f;
        int                   attachedAnimationIndex_ = -1;
        int                   dxlibAnimationIndex_    = -1; 
        float                 duration_secs_          = 0;
        float                 during_secs_            = 0;
        int                   modelAnimationIndex_    = 0;
        R4::Subject<UpdateCallbackContext> onUpdate_ = R4::Subject<UpdateCallbackContext>();
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
    archive(CEREAL_NVP(clipStartTime_));
    archive(CEREAL_NVP(clipEndTime_));
    archive(CEREAL_NVP(isLoop_));
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
    if (version >= 3) archive(CEREAL_NVP(clipStartTime_));
    if (version >= 3) archive(CEREAL_NVP(clipEndTime_));
    if (version >= 3) archive(CEREAL_NVP(isLoop_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationClipNode, 3);
#pragma endregion
