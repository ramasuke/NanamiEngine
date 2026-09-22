#pragma once
#include <string>
#include <vector>

#include "Ui_SwordMan_ActionInstructTutorialStep.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class IControlGuideFocusRequest;
}

namespace GamePlay::Ui
{
    // 戦闘訓練クエストの課題カード。操作ガイドが指している行のすぐ右に吸い付き、尾で行を指す。
    // 何を押すかは光っているガイドの行が示すので、本文にボタン名は書かない
    class SwordManActionInstructTutorial final : public Component::ComponentBase,
                                                 public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(GameCore::PlayerAvatar::SwordMan::IControlGuideFocusRequest& guideFocus);
        [[nodiscard]] std::size_t StepCount() const { return steps_.size(); }
        void ShowStep(std::size_t stepIndex);
        Coroutine::Task<void> PlayClearedAsync();
        void Hide();

    private:
        void OnUpdate() override;
        void CatchParts();
        void PresentText() const;
        void PresentFade() const;
        [[nodiscard]] std::string StepLabel(std::size_t stepIndex) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) card_;
        [[serialize(5)]] FIELD(NanamiUi::BlendImageRenderer) panel_;
        [[serialize(5)]] FIELD(NanamiUi::BlendImageRenderer) tail_;
        [[serialize(5)]] FIELD(NanamiUi::BlendImageRenderer) accent_;
        [[serialize(5)]] FIELD(NanamiUi::BlendImageRenderer) accentCleared_;
        [[serialize(5)]] FIELD(NanamiUi::BlendImageRenderer) clearMark_;
        [[serialize(5)]] FIELD(NanamiUi::TextRenderer) stepText_;
        [[serialize(5)]] FIELD(NanamiUi::TextRenderer) titleText_;
        [[serialize(5)]] FIELD(NanamiUi::TextRenderer) bodyText_;
        [[serialize(5)]] FIELD(NanamiUi::TextRenderer) clearText_;

        [[serialize(5)]] std::vector<ActionInstructTutorialStep> steps_;
        [[serialize(5)]] std::string stepLabelPrefix_ = "訓練";
        [[serialize(5)]] std::string clearedText_ = "よし！";

        /// 指された行の画面座標からカード中心までのずらし量
        [[serialize(5)]] glm::vec2 anchorOffset_px_ = glm::vec2(376.0f, 0.0f);
        [[serialize(5)]] glm::vec2 fallbackPos_px_ = glm::vec2(542.0f, 540.0f);
        [[serialize(5)]] float anchorFollowSpeed_pxPerSec_ = 900.0f;
        [[serialize(5)]] float appearDuration_secs_ = 0.25f;
        [[serialize(5)]] float appearSlide_px_ = 18.0f;
        [[serialize(5)]] float textFadeDuration_secs_ = 0.18f;
        [[serialize(5)]] float clearPopDuration_secs_ = 0.45f;
        [[serialize(5)]] float clearHoldDuration_secs_ = 0.9f;
        [[serialize(5)]] float clearMarkPopScale_ = 1.0f;
        [[serialize(5)]] float bodyAlphaRate_ = 0.86f;

        GameCore::PlayerAvatar::SwordMan::IControlGuideFocusRequest* guideFocus_ = nullptr;
        bool isPartsCaught_ = false;
        bool isShown_ = false;
        bool isCleared_ = false;
        std::size_t stepIndex_ = 0;
        float appearRate_ = 0.0f;
        float textRate_ = 0.0f;
        float clearRate_ = 0.0f;
        glm::vec2 cardPos_px_ = glm::vec2(0.0f);
        glm::vec3 cardBasePos_ = glm::vec3(0.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(card_));
            archive(CEREAL_NVP(panel_));
            archive(CEREAL_NVP(tail_));
            archive(CEREAL_NVP(accent_));
            archive(CEREAL_NVP(accentCleared_));
            archive(CEREAL_NVP(clearMark_));
            archive(CEREAL_NVP(stepText_));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(bodyText_));
            archive(CEREAL_NVP(clearText_));

            const int stepCount = static_cast<int>(steps_.size());
            archive(cereal::make_nvp("stepCount", stepCount));
            for (int i = 0; i < stepCount; ++i)
                archive(cereal::make_nvp(("step_" + std::to_string(i)).c_str(), steps_[i]));

            archive(CEREAL_NVP(stepLabelPrefix_));
            archive(CEREAL_NVP(clearedText_));
            archive(CEREAL_NVP(anchorOffset_px_));
            archive(CEREAL_NVP(fallbackPos_px_));
            archive(CEREAL_NVP(anchorFollowSpeed_pxPerSec_));
            archive(CEREAL_NVP(appearDuration_secs_));
            archive(CEREAL_NVP(appearSlide_px_));
            archive(CEREAL_NVP(textFadeDuration_secs_));
            archive(CEREAL_NVP(clearPopDuration_secs_));
            archive(CEREAL_NVP(clearHoldDuration_secs_));
            archive(CEREAL_NVP(clearMarkPopScale_));
            archive(CEREAL_NVP(bodyAlphaRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(card_));
            archive(CEREAL_NVP(panel_));
            archive(CEREAL_NVP(tail_));
            archive(CEREAL_NVP(accent_));
            archive(CEREAL_NVP(accentCleared_));
            archive(CEREAL_NVP(clearMark_));
            archive(CEREAL_NVP(stepText_));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(bodyText_));
            archive(CEREAL_NVP(clearText_));

            int stepCount = 0;
            archive(cereal::make_nvp("stepCount", stepCount));
            steps_.resize(stepCount);
            for (int i = 0; i < stepCount; ++i)
                archive(cereal::make_nvp(("step_" + std::to_string(i)).c_str(), steps_[i]));

            archive(CEREAL_NVP(stepLabelPrefix_));
            archive(CEREAL_NVP(clearedText_));
            archive(CEREAL_NVP(anchorOffset_px_));
            archive(CEREAL_NVP(fallbackPos_px_));
            archive(CEREAL_NVP(anchorFollowSpeed_pxPerSec_));
            archive(CEREAL_NVP(appearDuration_secs_));
            archive(CEREAL_NVP(appearSlide_px_));
            archive(CEREAL_NVP(textFadeDuration_secs_));
            archive(CEREAL_NVP(clearPopDuration_secs_));
            archive(CEREAL_NVP(clearHoldDuration_secs_));
            archive(CEREAL_NVP(clearMarkPopScale_));
            archive(CEREAL_NVP(bodyAlphaRate_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SwordManActionInstructTutorial, 5)
