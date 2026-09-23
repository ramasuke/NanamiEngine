#pragma once
#include "vec2.hpp"
#include "../../../Asset/MV1/MV1File.h"
#include "../../../../Core/Object/Field/Field.h"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../IAnimationNode.h"
#include "../LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::AnimationTree
{
    constexpr auto ENTRY_NODE_SIZE = glm::vec2(120.0f, 60.0f);
    
    class AnimatorEntryNode final : public IAnimationNode
    {
    public:
        void InitForGamePlay(int modelHandle) override;
        [[nodiscard]] const Guid& GetGuid()  const override { return guid_;         }
        [[nodiscard]] glm::vec2   Position() const override { return position_;     }
        void SetPosition(const glm::vec2& position) override { position_ = position; }
        [[nodiscard]] std::string GraphNodeName  () const override { return "Entry"; }
        [[nodiscard]] std::string GraphNodeDetail() const override { return "開始"; }
        [[nodiscard]] float GetAnimDuration_secs() const override { return 0; }
        void OnUpdateBlendRate(float  blendRate) override;
        void OnUpdateAnimation(int  modelHandle, float timeScale) override;
        R4::Observable<UpdateCallbackContext> OnUpdated() override { return onUpdate_.AsObservable(); }
        void OnExitNode(int modelHandle) override;
        
    private:
        glm::vec2 position_;
        Guid guid_;
        float speed_ = 1.0f;
        R4::Subject<UpdateCallbackContext> onUpdate_ = R4::Subject<UpdateCallbackContext>();
        int animationModelHandle_ = -1;
        bool isDrag_ = false;
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
#pragma endregion
