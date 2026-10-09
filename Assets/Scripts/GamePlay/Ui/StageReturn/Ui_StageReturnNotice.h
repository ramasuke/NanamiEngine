#pragma once
#include <cstdint>
#include <memory>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace GamePlay::Ui
{
    class StageReturnNoticeUi final : public Component::ComponentBase,
                                      public LifeCycleCallback::IStartable,
                                      public LifeCycleCallback::IUpdatable
    {
    public:
        static constexpr int RETURN_INDEX   = 0;
        static constexpr int STAY_INDEX     = 1;
        static constexpr int SETTINGS_INDEX = 2;
        static constexpr int ROW_COUNT      = 3;

        void Open(bool isHostLeaving, int selection);
        void Hide();
        void SetSelection(int index);

        [[nodiscard]] bool IsShown() const { return phase_ != Phase::Hidden; }
        [[nodiscard]] std::shared_ptr<NanamiUi::Button> ConfirmButton() const { return confirmButton_.get(); }
        [[nodiscard]] std::shared_ptr<NanamiUi::Button> CancelButton() const { return cancelButton_.get(); }

    private:
        enum class Phase : std::uint8_t
        {
            Hidden,
            Entering,
            Shown,
        };

        void OnStart () override;
        void OnUpdate() override;

        // NOTE: 呼び出し側の OnStart が先に走って Open されても、元の位置を取り損ねないように
        void EnsureStarted();
        void UpdateEnter(float deltaSecs);
        void UpdateStamp(float deltaSecs);
        void PressStamp(const std::shared_ptr<NanamiUi::BlendImageRenderer>& stamp);
        void ApplyVeil(float rate) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) noticeRoot_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) veilBlack_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) veil_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) soloNotice_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) hostNotice_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) hostNoteText_;

        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) returnLabel_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) returnUnderline_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) returnStamp_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) stayLabel_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) stayUnderline_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) stayStamp_;
        [[serialize(1)]] FIELD(NanamiUi::TextRenderer) settingsLabel_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) settingsUnderline_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) settingsStamp_;

        [[serialize(0)]] FIELD(NanamiUi::Button) confirmButton_;
        [[serialize(0)]] FIELD(NanamiUi::Button) cancelButton_;

        [[serialize(0)]] Color32 selectedColor_   = Color32(48, 30, 20);
        [[serialize(0)]] Color32 unselectedColor_ = Color32(104, 78, 54);
        [[serialize(0)]] int   veilBlendRate_       = 200;
        [[serialize(0)]] int   veilBlackBlendRate_  = 90;
        [[serialize(0)]] float dropDistance_px_     = 40.0f;
        [[serialize(0)]] float enterDuration_secs_  = 0.2f;
        [[serialize(0)]] float stampDuration_secs_  = 0.3f;
        [[serialize(0)]] float stampStartScale_     = 1.6f;

        bool  isStarted_ = false;
        Phase phase_ = Phase::Hidden;
        glm::vec3 noticeBasePos_ = glm::vec3(0.0f);
        LibCore::Tween::TweenPlayer<float> enterTween_;

        std::weak_ptr<NanamiUi::BlendImageRenderer> pressingStamp_;
        glm::vec3 stampBaseScale_ = glm::vec3(1.0f);
        LibCore::Tween::TweenPlayer<float> stampScaleTween_;
        LibCore::Tween::TweenPlayer<float> stampAlphaTween_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(noticeRoot_));
            archive(CEREAL_NVP(veilBlack_));
            archive(CEREAL_NVP(veil_));
            archive(CEREAL_NVP(soloNotice_));
            archive(CEREAL_NVP(hostNotice_));
            archive(CEREAL_NVP(hostNoteText_));
            archive(CEREAL_NVP(returnLabel_));
            archive(CEREAL_NVP(returnUnderline_));
            archive(CEREAL_NVP(returnStamp_));
            archive(CEREAL_NVP(stayLabel_));
            archive(CEREAL_NVP(stayUnderline_));
            archive(CEREAL_NVP(stayStamp_));
            archive(CEREAL_NVP(confirmButton_));
            archive(CEREAL_NVP(cancelButton_));
            archive(CEREAL_NVP(selectedColor_));
            archive(CEREAL_NVP(unselectedColor_));
            archive(CEREAL_NVP(veilBlendRate_));
            archive(CEREAL_NVP(veilBlackBlendRate_));
            archive(CEREAL_NVP(dropDistance_px_));
            archive(CEREAL_NVP(enterDuration_secs_));
            archive(CEREAL_NVP(stampDuration_secs_));
            archive(CEREAL_NVP(stampStartScale_));
            archive(CEREAL_NVP(settingsLabel_));
            archive(CEREAL_NVP(settingsUnderline_));
            archive(CEREAL_NVP(settingsStamp_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(noticeRoot_));
            if (version >= 0) archive(CEREAL_NVP(veilBlack_));
            if (version >= 0) archive(CEREAL_NVP(veil_));
            if (version >= 0) archive(CEREAL_NVP(soloNotice_));
            if (version >= 0) archive(CEREAL_NVP(hostNotice_));
            if (version >= 0) archive(CEREAL_NVP(hostNoteText_));
            if (version >= 0) archive(CEREAL_NVP(returnLabel_));
            if (version >= 0) archive(CEREAL_NVP(returnUnderline_));
            if (version >= 0) archive(CEREAL_NVP(returnStamp_));
            if (version >= 0) archive(CEREAL_NVP(stayLabel_));
            if (version >= 0) archive(CEREAL_NVP(stayUnderline_));
            if (version >= 0) archive(CEREAL_NVP(stayStamp_));
            if (version >= 0) archive(CEREAL_NVP(confirmButton_));
            if (version >= 0) archive(CEREAL_NVP(cancelButton_));
            if (version >= 0) archive(CEREAL_NVP(selectedColor_));
            if (version >= 0) archive(CEREAL_NVP(unselectedColor_));
            if (version >= 0) archive(CEREAL_NVP(veilBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(veilBlackBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(dropDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(enterDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(stampDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(stampStartScale_));
            if (version >= 1) archive(CEREAL_NVP(settingsLabel_));
            if (version >= 1) archive(CEREAL_NVP(settingsUnderline_));
            if (version >= 1) archive(CEREAL_NVP(settingsStamp_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageReturnNoticeUi, 1);
