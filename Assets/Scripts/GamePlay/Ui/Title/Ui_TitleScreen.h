#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace GamePlay::Ui
{
    /**
     * @brief タイトル画面の見た目。黒から明ける → 題字が浮かぶ →「ボタンを押してください」が息をする → メニュー。
     *
     * メニューは右寄せの文字列で、選んでいる行の後ろに墨を掃いた帯 (selectBand_) を滑らせる。
     * 入力は TitleScreenPresenter が読む。時間は壁時計で進める (シーン切り替え中の DeltaTime 0 に引きずられない)
     */
    class TitleScreenUi final : public Component::ComponentBase,
                                public LifeCycleCallback::IStartable,
                                public LifeCycleCallback::IUpdatable
    {
    public:
        static constexpr int START_INDEX = 0;
        static constexpr int EXIT_INDEX  = 1;
        static constexpr int MENU_COUNT  = 2;

        /** @brief 出だしの演出を飛ばして、題字と「ボタンを押してください」まで出し切る */
        void SkipIntro();
        void ShowMenu();
        /** @brief メニューを畳んで「ボタンを押してください」に戻る */
        void HideMenu();
        void SetSelection(int index);
        void SetStartLabel(const std::string& label) const;

        [[nodiscard]] bool IsIntroFinished() const { return phase_ != Phase::Intro; }
        [[nodiscard]] bool IsMenuReady() const { return phase_ == Phase::Menu && menuElapsed_secs_ >= menuInputGuard_secs_; }
        [[nodiscard]] std::shared_ptr<NanamiUi::Button> MenuButton(int index) const;

    private:
        enum class Phase : std::uint8_t
        {
            Intro,
            PressWaiting,
            Menu,
        };

        void OnStart () override;
        void OnUpdate() override;
        [[nodiscard]] float TickWallClockSeconds();

        void ApplyIntro() const;
        void ApplyPress(float deltaSecs);
        void ApplyMenu(float deltaSecs);
        [[nodiscard]] std::shared_ptr<NanamiUi::TextRenderer> MenuText(int index) const;

        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) veil_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) logo_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) pressText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) pressDeco_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) startText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) exitText_;
        [[serialize(0)]] FIELD(NanamiUi::Button) startButton_;
        [[serialize(0)]] FIELD(NanamiUi::Button) exitButton_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) selectBand_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) moveHintTag_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) moveHintText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) confirmHintTag_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) confirmHintText_;

        [[serialize(0)]] float veilOpen_secs_ = 2.2f;
        [[serialize(0)]] float logoDelay_secs_ = 1.4f;
        [[serialize(0)]] float logoFade_secs_ = 1.8f;
        [[serialize(0)]] float pressDelay_secs_ = 3.2f;
        [[serialize(0)]] float pressFade_secs_ = 0.8f;
        // 「ボタンを押してください」がゆっくり明滅する周期
        [[serialize(0)]] float pressPulsePeriod_secs_ = 2.6f;
        [[serialize(0)]] float pressPulseMinRate_ = 0.35f;
        [[serialize(0)]] float menuFade_secs_ = 0.3f;
        [[serialize(0)]] float menuStagger_secs_ = 0.07f;
        [[serialize(0)]] float menuSlide_px_ = 24.0f;
        [[serialize(0)]] float menuInputGuard_secs_ = 0.2f;
        [[serialize(0)]] float bandFollowRate_ = 18.0f;
        [[serialize(0)]] float unselectedTextRate_ = 0.62f;

        Phase phase_ = Phase::Intro;
        float introElapsed_secs_ = 0.0f;
        float pressElapsed_secs_ = 0.0f;
        float menuElapsed_secs_ = 0.0f;
        /** 1 でメニューが出切り、0 で畳み切り */
        float menuRate_ = 0.0f;
        bool isMenuShown_ = false;
        int selection_ = START_INDEX;
        std::array<glm::vec3, MENU_COUNT> menuBasePos_{};
        glm::vec3 bandBasePos_ = glm::vec3(0.0f);
        float bandOffsetY_ = 0.0f;
        /** 帯が今いる行の文字の y (上端) */
        float bandY_ = 0.0f;
        int lastTickMs_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(veil_));
            archive(CEREAL_NVP(logo_));
            archive(CEREAL_NVP(pressText_));
            archive(CEREAL_NVP(pressDeco_));
            archive(CEREAL_NVP(startText_));
            archive(CEREAL_NVP(exitText_));
            archive(CEREAL_NVP(startButton_));
            archive(CEREAL_NVP(exitButton_));
            archive(CEREAL_NVP(selectBand_));
            archive(CEREAL_NVP(moveHintTag_));
            archive(CEREAL_NVP(moveHintText_));
            archive(CEREAL_NVP(confirmHintTag_));
            archive(CEREAL_NVP(confirmHintText_));
            archive(CEREAL_NVP(veilOpen_secs_));
            archive(CEREAL_NVP(logoDelay_secs_));
            archive(CEREAL_NVP(logoFade_secs_));
            archive(CEREAL_NVP(pressDelay_secs_));
            archive(CEREAL_NVP(pressFade_secs_));
            archive(CEREAL_NVP(pressPulsePeriod_secs_));
            archive(CEREAL_NVP(pressPulseMinRate_));
            archive(CEREAL_NVP(menuFade_secs_));
            archive(CEREAL_NVP(menuStagger_secs_));
            archive(CEREAL_NVP(menuSlide_px_));
            archive(CEREAL_NVP(menuInputGuard_secs_));
            archive(CEREAL_NVP(bandFollowRate_));
            archive(CEREAL_NVP(unselectedTextRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(veil_));
            if (version >= 0) archive(CEREAL_NVP(logo_));
            if (version >= 0) archive(CEREAL_NVP(pressText_));
            if (version >= 0) archive(CEREAL_NVP(pressDeco_));
            if (version >= 0) archive(CEREAL_NVP(startText_));
            if (version >= 0) archive(CEREAL_NVP(exitText_));
            if (version >= 0) archive(CEREAL_NVP(startButton_));
            if (version >= 0) archive(CEREAL_NVP(exitButton_));
            if (version >= 0) archive(CEREAL_NVP(selectBand_));
            if (version >= 0) archive(CEREAL_NVP(moveHintTag_));
            if (version >= 0) archive(CEREAL_NVP(moveHintText_));
            if (version >= 0) archive(CEREAL_NVP(confirmHintTag_));
            if (version >= 0) archive(CEREAL_NVP(confirmHintText_));
            if (version >= 0) archive(CEREAL_NVP(veilOpen_secs_));
            if (version >= 0) archive(CEREAL_NVP(logoDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(logoFade_secs_));
            if (version >= 0) archive(CEREAL_NVP(pressDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(pressFade_secs_));
            if (version >= 0) archive(CEREAL_NVP(pressPulsePeriod_secs_));
            if (version >= 0) archive(CEREAL_NVP(pressPulseMinRate_));
            if (version >= 0) archive(CEREAL_NVP(menuFade_secs_));
            if (version >= 0) archive(CEREAL_NVP(menuStagger_secs_));
            if (version >= 0) archive(CEREAL_NVP(menuSlide_px_));
            if (version >= 0) archive(CEREAL_NVP(menuInputGuard_secs_));
            if (version >= 0) archive(CEREAL_NVP(bandFollowRate_));
            if (version >= 0) archive(CEREAL_NVP(unselectedTextRate_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::TitleScreenUi, 0);
