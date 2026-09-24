#pragma once
#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Scene/SceneFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ImageRenderer/Animation/ImageAnimationRenderer.h"
#include "Engine/Module/NanamiUI/MovieRenderer/MovieRenderer.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Stage/Ui_StageSelect_StageUI.h"
#include "MapMarker/StageMapMarker.h"
#include "Room/Ui_StageSelect_RoomUi.h"
#include "../cereal/include/cereal/types/vector.hpp"

namespace GamePlay::Ui
{
    class StageSelectUi final : public Component::ComponentBase,
                                public LifeCycleCallback::IStartable
    {
    public:
        [[nodiscard]] std::vector<std::weak_ptr<StageSelectStageUi>> Stages() const;
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiUi::MouseState> OnWorldEnterButtonClicked() const { return worldEnterButton_->OnClick(); }

        void HighlightSelectedStage(size_t selectedIndex);
        void SetWorldEnterButtonEnabled(bool isEnabled);
        /** @brief 選んだステージへの遷移 */
        void EnterWorld(GameCore::Scene::Main::SceneType sceneType);
        void ShowMapMarker(const glm::vec2& position, bool isCleared);
        void HideMapMarker();
        void ShowStageDetail(const Asset::StageData& stage);
        /** @brief 未解放のステージ。中身は伏せて、解放条件の文言だけ出す */
        void ShowLockedStageDetail(const Asset::StageData& stage);
        void ShowNoSelectionDetail();
        [[nodiscard]] std::shared_ptr<StageSelectRoomUi> Room() const { return roomUi_.get(); }

    private:
        void OnStart() override;
        void OnDestroy() override;
        Coroutine::Task<void> StartStageSelectAsync();
        Coroutine::Task<void> AppearBackGroundMaskAsync();
        static Coroutine::Task<void> FadeBlendRateAsync(std::weak_ptr<NanamiUi::BlendImageRenderer> renderer, int from, int to);
        static Coroutine::Task<void> FadeBlendRateAsync(std::weak_ptr<NanamiUi::MovieRenderer> renderer, int from, int to);
        void SetDetailDifficultyVisible(bool isVisible);

        [[serialize(0)]] FIELD(Asset::SoundFile) bgm_;

        [[serialize(6)]] FIELD(NanamiUi::BlendImageRenderer) backGroundMask_;
        [[serialize(0)]] int backGroundMaskBlendRate_ = 55;
        [[serialize(6)]] std::vector<FIELD(StageSelectStageUi)> stageSelectButtons_;

        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) stageSelectBackGroundMask_;
        [[serialize(1)]] int stageSelectBackGroundMaskBlendRate_ = 50;
        [[serialize(6)]] FIELD(NanamiUi::MovieRenderer) worldMovieRenderer_;
        bool isEnteringWorld_ = false;
        [[serialize(6)]] FIELD(NanamiUi::Button) worldEnterButton_;
        [[serialize(6)]] FIELD(NanamiUi::ImageAnimationRenderer) worldEnterButtonGlow_;
        [[serialize(6)]] FIELD(NanamiUi::MovieRenderer) backGround_;
        [[serialize(3)]] FIELD(Asset::SpriteFile) worldEnterButtonActiveSprite_;
        [[serialize(3)]] FIELD(Asset::SpriteFile) worldEnterButtonDisabledSprite_;
        [[serialize(6)]] FIELD(StageMapMarker) mapMarker_;
        [[serialize(6)]] FIELD(Component::ImageRenderer) detailPreview_;
        [[serialize(6)]] FIELD(Component::ImageRenderer) detailElement_;
        [[serialize(6)]] FIELD(NanamiUi::TextRenderer) detailLabel_;
        [[serialize(6)]] FIELD(NanamiUi::TextRenderer) detailTitle_;
        [[serialize(6)]] FIELD(NanamiUi::TextRenderer) detailTag_;
        [[serialize(6)]] FIELD(StageDifficultyPips) detailDifficulty_;
        [[serialize(6)]] std::vector<FIELD(NanamiUi::TextRenderer)> detailDescriptionLines_;
        [[serialize(5)]] std::string noSelectionTitle_;
        [[serialize(7)]] FIELD(StageSelectRoomUi) roomUi_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(bgm_));
            archive(CEREAL_NVP(backGroundMask_));
            archive(CEREAL_NVP(backGroundMaskBlendRate_));
            archive(CEREAL_NVP(stageSelectButtons_));
            archive(CEREAL_NVP(stageSelectBackGroundMask_));
            archive(CEREAL_NVP(stageSelectBackGroundMaskBlendRate_));
            archive(CEREAL_NVP(worldMovieRenderer_));
            archive(CEREAL_NVP(worldEnterButton_));
            archive(CEREAL_NVP(worldEnterButtonGlow_));
            archive(CEREAL_NVP(backGround_));
            archive(CEREAL_NVP(worldEnterButtonActiveSprite_));
            archive(CEREAL_NVP(worldEnterButtonDisabledSprite_));
            archive(CEREAL_NVP(mapMarker_));
            archive(CEREAL_NVP(detailPreview_));
            archive(CEREAL_NVP(detailElement_));
            archive(CEREAL_NVP(detailLabel_));
            archive(CEREAL_NVP(detailTitle_));
            archive(CEREAL_NVP(detailTag_));
            archive(CEREAL_NVP(detailDifficulty_));
            archive(CEREAL_NVP(detailDescriptionLines_));
            archive(CEREAL_NVP(noSelectionTitle_));
            archive(CEREAL_NVP(roomUi_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bgm_));
            if (version >= 6) archive(CEREAL_NVP(backGroundMask_));
            if (version >= 0) archive(CEREAL_NVP(backGroundMaskBlendRate_));
            if (version >= 6) archive(CEREAL_NVP(stageSelectButtons_));
            if (version >= 1) archive(CEREAL_NVP(stageSelectBackGroundMask_));
            if (version >= 1) archive(CEREAL_NVP(stageSelectBackGroundMaskBlendRate_));
            if (version >= 6) archive(CEREAL_NVP(worldMovieRenderer_));
            if (version >= 6) archive(CEREAL_NVP(worldEnterButton_));
            if (version >= 6) archive(CEREAL_NVP(worldEnterButtonGlow_));
            if (version >= 6) archive(CEREAL_NVP(backGround_));
            if (version >= 3) archive(CEREAL_NVP(worldEnterButtonActiveSprite_));
            if (version >= 3) archive(CEREAL_NVP(worldEnterButtonDisabledSprite_));
            if (version >= 6) archive(CEREAL_NVP(mapMarker_));
            if (version >= 6) archive(CEREAL_NVP(detailPreview_));
            if (version >= 6) archive(CEREAL_NVP(detailElement_));
            if (version >= 6) archive(CEREAL_NVP(detailLabel_));
            if (version >= 6) archive(CEREAL_NVP(detailTitle_));
            if (version >= 6) archive(CEREAL_NVP(detailTag_));
            if (version >= 6) archive(CEREAL_NVP(detailDifficulty_));
            if (version >= 6) archive(CEREAL_NVP(detailDescriptionLines_));
            if (version >= 5) archive(CEREAL_NVP(noSelectionTitle_));
            if (version >= 7) archive(CEREAL_NVP(roomUi_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectUi, 7);
