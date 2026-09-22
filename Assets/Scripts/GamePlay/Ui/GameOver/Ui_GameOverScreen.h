#pragma once
#include <cstdint>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Ui_GameOverButton.h"

namespace GamePlay::Ui
{
    /**
     * @brief 力尽きたときに出す石版と選択肢の見た目。
     *
     * GameOverScene.scene に常駐させる（ロード画面と同じく、メインシーンを入れ替えても残る）。
     * 黒い幕が降りる → 石版がせり上がって着地する → 鉄札が並ぶ、の順に出す。
     * 抜けるときは幕を真っ黒まで下ろし、遷移が済んだら OpenCurtain で明ける。
     * ChangeMainScene の間は Time::DeltaTime() が 0 になるので、時間は壁時計で進める
     */
    class GameOverScreenUi final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable,
                                   public LifeCycleCallback::IUpdatable
    {
    public:
        static constexpr int RETRY_INDEX = 0;
        static constexpr int TITLE_INDEX = 1;

        void Show();
        void SetSelection(int index);
        void BeginCurtain();
        void OpenCurtain();
        void HideImmediately();

        [[nodiscard]] bool IsShown() const { return phase_ != Phase::Hidden; }
        [[nodiscard]] bool IsInputReady() const { return phase_ == Phase::Waiting; }
        [[nodiscard]] bool IsCurtainClosed() const { return phase_ == Phase::Closed; }
        [[nodiscard]] std::shared_ptr<GameOverButton> ChoiceButton(int index) const;

    private:
        enum class Phase : std::uint8_t
        {
            Hidden,
            Intro,
            Waiting,
            Closing,
            Closed,
            Opening,
        };

        void OnStart() override;
        void OnUpdate() override;
        /** @brief timeScale にも SkipNextFrame にも影響されない壁時計の差分を返す */
        [[nodiscard]] float TickWallClockSeconds();

        void UpdateIntro();
        void UpdateCurtain(float deltaSecs);
        void TickButtons(float deltaSecs) const;
        /** @param appearRate 石版・鉄札・操作ヒントの見え方。幕とは別に下ろす */
        void ApplyContentAlpha(float appearRate) const;
        void ApplyVeil(float blendRate) const;
        void SetSlabOffset(float offsetY) const;
        void SetButtonOffset(const std::shared_ptr<GameOverButton>& button, const glm::vec3& basePos, float offsetY) const;
        void SetVisualEnabled(bool isEnabled) const;
        void PlaySe(const std::shared_ptr<Asset::SoundFile>& sound) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) veil_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) slab_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) slabDirt_;
        [[serialize(0)]] FIELD(GameOverButton) retryButton_;
        [[serialize(0)]] FIELD(GameOverButton) titleButton_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) moveHintTag_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) moveHintText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) confirmHintTag_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) confirmHintText_;
        [[serialize(0)]] FIELD(Asset::SoundFile) stingSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) slabLandSound_;

        [[serialize(0)]] int veilBlendRate_ = 165;
        [[serialize(0)]] float veilFadeSecs_ = 1.6f;
        [[serialize(0)]] float stingDelaySecs_ = 0.35f;
        [[serialize(0)]] float slabDelaySecs_ = 1.9f;
        [[serialize(0)]] float slabRiseSecs_ = 0.42f;
        [[serialize(0)]] float slabRiseDistance_px_ = 90.0f;
        [[serialize(0)]] float slabOvershoot_px_ = 14.0f;
        [[serialize(0)]] float dirtFadeSecs_ = 0.12f;
        [[serialize(0)]] float buttonsDelaySecs_ = 2.55f;
        [[serialize(0)]] float buttonsRiseSecs_ = 0.32f;
        [[serialize(0)]] float buttonStaggerSecs_ = 0.08f;
        [[serialize(0)]] float buttonRiseDistance_px_ = 26.0f;
        [[serialize(0)]] float inputGuardSecs_ = 0.35f;
        [[serialize(0)]] float curtainCloseSecs_ = 0.45f;
        [[serialize(0)]] float curtainOpenSecs_ = 0.6f;

        Phase phase_ = Phase::Hidden;
        float elapsedSecs_ = 0.0f;
        float curtainElapsedSecs_ = 0.0f;
        float curtainFromVeil_ = 0.0f;
        int selection_ = RETRY_INDEX;
        bool isStingPlayed_ = false;
        bool isSlabLanded_ = false;
        glm::vec3 slabBasePos_ = glm::vec3(0.0f);
        glm::vec3 retryBasePos_ = glm::vec3(0.0f);
        glm::vec3 titleBasePos_ = glm::vec3(0.0f);
        int lastTickMs_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(veil_));
            archive(CEREAL_NVP(slab_));
            archive(CEREAL_NVP(slabDirt_));
            archive(CEREAL_NVP(retryButton_));
            archive(CEREAL_NVP(titleButton_));
            archive(CEREAL_NVP(moveHintTag_));
            archive(CEREAL_NVP(moveHintText_));
            archive(CEREAL_NVP(confirmHintTag_));
            archive(CEREAL_NVP(confirmHintText_));
            archive(CEREAL_NVP(stingSound_));
            archive(CEREAL_NVP(slabLandSound_));
            archive(CEREAL_NVP(veilBlendRate_));
            archive(CEREAL_NVP(veilFadeSecs_));
            archive(CEREAL_NVP(stingDelaySecs_));
            archive(CEREAL_NVP(slabDelaySecs_));
            archive(CEREAL_NVP(slabRiseSecs_));
            archive(CEREAL_NVP(slabRiseDistance_px_));
            archive(CEREAL_NVP(slabOvershoot_px_));
            archive(CEREAL_NVP(dirtFadeSecs_));
            archive(CEREAL_NVP(buttonsDelaySecs_));
            archive(CEREAL_NVP(buttonsRiseSecs_));
            archive(CEREAL_NVP(buttonStaggerSecs_));
            archive(CEREAL_NVP(buttonRiseDistance_px_));
            archive(CEREAL_NVP(inputGuardSecs_));
            archive(CEREAL_NVP(curtainCloseSecs_));
            archive(CEREAL_NVP(curtainOpenSecs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(veil_));
            if (version >= 0) archive(CEREAL_NVP(slab_));
            if (version >= 0) archive(CEREAL_NVP(slabDirt_));
            if (version >= 0) archive(CEREAL_NVP(retryButton_));
            if (version >= 0) archive(CEREAL_NVP(titleButton_));
            if (version >= 0) archive(CEREAL_NVP(moveHintTag_));
            if (version >= 0) archive(CEREAL_NVP(moveHintText_));
            if (version >= 0) archive(CEREAL_NVP(confirmHintTag_));
            if (version >= 0) archive(CEREAL_NVP(confirmHintText_));
            if (version >= 0) archive(CEREAL_NVP(stingSound_));
            if (version >= 0) archive(CEREAL_NVP(slabLandSound_));
            if (version >= 0) archive(CEREAL_NVP(veilBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(veilFadeSecs_));
            if (version >= 0) archive(CEREAL_NVP(stingDelaySecs_));
            if (version >= 0) archive(CEREAL_NVP(slabDelaySecs_));
            if (version >= 0) archive(CEREAL_NVP(slabRiseSecs_));
            if (version >= 0) archive(CEREAL_NVP(slabRiseDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(slabOvershoot_px_));
            if (version >= 0) archive(CEREAL_NVP(dirtFadeSecs_));
            if (version >= 0) archive(CEREAL_NVP(buttonsDelaySecs_));
            if (version >= 0) archive(CEREAL_NVP(buttonsRiseSecs_));
            if (version >= 0) archive(CEREAL_NVP(buttonStaggerSecs_));
            if (version >= 0) archive(CEREAL_NVP(buttonRiseDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(inputGuardSecs_));
            if (version >= 0) archive(CEREAL_NVP(curtainCloseSecs_));
            if (version >= 0) archive(CEREAL_NVP(curtainOpenSecs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameOverScreenUi, 0)
