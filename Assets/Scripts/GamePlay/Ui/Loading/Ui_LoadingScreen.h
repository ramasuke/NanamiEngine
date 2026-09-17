#pragma once
#include <memory>
#include <string>

#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../Data/Stage/Data_StageData.h"
#include "../../../Core/Game/Scene/Main/Loading/Main_SceneLoadStep.h"
#include "../StageSelect/Difficulty/StageDifficultyPips.h"
#include "Hint/Ui_LoadingHintCard.h"

namespace GamePlay::Ui
{
    /**
     * @brief ステージ遷移中に出す全画面ロード画面。
     *
     * GameManage.scene に常駐させる。ロード中は新規オブジェクトの暖機が止まりうるため、
     * ロード開始より前から登録済みでないと動かない。
     * アニメーションは Time::DeltaTime() を使わない。ChangeMainScene が SkipNextFrame を
     * 60 回積むので、その間 DeltaTime() は 0 を返し続ける
     */
    class LoadingScreenUi final : public Component::ComponentBase,
                                  public LifeCycleCallback::IStartable,
                                  public LifeCycleCallback::IUpdatable
    {
    public:
        /** @brief 表示を開始する。RequestChangeScene より前に呼ぶこと */
        void Show(const std::shared_ptr<Asset::StageData>& stageData);
        void SetStep(GameCore::Scene::Main::SceneLoadStep step);
        /** @brief 読み込みに失敗したことを表示する */
        void Fail(const std::string& message);
        /** @brief 最低表示時間と進捗の詰めが終わり次第、消えていく */
        void BeginHide();
        [[nodiscard]] bool IsShown() const { return phase_ != Phase::Hidden; }
        /** @brief カバーが画面を覆い切るまで待つ */
        [[nodiscard]] Coroutine::Task<void> WaitCoverOpaqueAsync() const;

    private:
        enum class Phase : std::uint8_t
        {
            Hidden,
            FadingIn,
            Visible,
            FadingOut,
        };

        void OnStart() override;
        void OnUpdate() override;
        /** @brief timeScale にも SkipNextFrame にも影響されない壁時計の差分を返す */
        [[nodiscard]] float TickWallClockSeconds();
        void ApplyStageData(const Asset::StageData& stageData) const;
        void UpdateCoverFade(float deltaSecs);
        void UpdateProgress(float deltaSecs);
        void SetVisualEnabled(bool isEnabled) const;
        void ApplyCoverBlendRate() const;
        [[nodiscard]] float CalcRawProgress() const;
        [[nodiscard]] bool CanHide() const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) cover_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) backdrop_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) cardThumbnail_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) cardElement_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) stageNameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) tagText_;
        [[serialize(0)]] FIELD(StageDifficultyPips) difficultyPips_;
        [[serialize(0)]] FIELD(NanamiUi::Slider) progressBar_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) statusText_;
        [[serialize(0)]] FIELD(LoadingHintCard) hintCard_;
        [[serialize(0)]] float fadeInSecs_ = 0.30f;
        [[serialize(0)]] float fadeOutSecs_ = 0.40f;
        [[serialize(0)]] float minShowSecs_ = 1.20f;
        [[serialize(0)]] float progressFollowRate_ = 6.0f;
        [[serialize(0)]] float finishSecs_ = 0.25f;
        [[serialize(0)]] int backdropBlendRate_ = 110;

        Phase phase_ = Phase::Hidden;
        GameCore::Scene::Main::SceneLoadStep step_ = GameCore::Scene::Main::SceneLoadStep::Idle;
        std::string statusMessage_;
        float stepElapsedSecs_ = 0.0f;
        float shownElapsedSecs_ = 0.0f;
        float coverBlendRate_ = 0.0f;
        float displayedProgress_ = 0.0f;
        int lastShownPercent_ = -1;
        int lastTickMs_ = 0;
        bool isHideRequested_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(cover_));
            archive(CEREAL_NVP(backdrop_));
            archive(CEREAL_NVP(cardThumbnail_));
            archive(CEREAL_NVP(cardElement_));
            archive(CEREAL_NVP(stageNameText_));
            archive(CEREAL_NVP(tagText_));
            archive(CEREAL_NVP(difficultyPips_));
            archive(CEREAL_NVP(progressBar_));
            archive(CEREAL_NVP(statusText_));
            archive(CEREAL_NVP(hintCard_));
            archive(CEREAL_NVP(fadeInSecs_));
            archive(CEREAL_NVP(fadeOutSecs_));
            archive(CEREAL_NVP(minShowSecs_));
            archive(CEREAL_NVP(progressFollowRate_));
            archive(CEREAL_NVP(finishSecs_));
            archive(CEREAL_NVP(backdropBlendRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(cover_));
            if (version >= 0) archive(CEREAL_NVP(backdrop_));
            if (version >= 0) archive(CEREAL_NVP(cardThumbnail_));
            if (version >= 0) archive(CEREAL_NVP(cardElement_));
            if (version >= 0) archive(CEREAL_NVP(stageNameText_));
            if (version >= 0) archive(CEREAL_NVP(tagText_));
            if (version >= 0) archive(CEREAL_NVP(difficultyPips_));
            if (version >= 0) archive(CEREAL_NVP(progressBar_));
            if (version >= 0) archive(CEREAL_NVP(statusText_));
            if (version >= 0) archive(CEREAL_NVP(hintCard_));
            if (version >= 0) archive(CEREAL_NVP(fadeInSecs_));
            if (version >= 0) archive(CEREAL_NVP(fadeOutSecs_));
            if (version >= 0) archive(CEREAL_NVP(minShowSecs_));
            if (version >= 0) archive(CEREAL_NVP(progressFollowRate_));
            if (version >= 0) archive(CEREAL_NVP(finishSecs_));
            if (version >= 0) archive(CEREAL_NVP(backdropBlendRate_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LoadingScreenUi, 0)
