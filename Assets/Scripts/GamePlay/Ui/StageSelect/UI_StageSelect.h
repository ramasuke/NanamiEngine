#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/Component/ImageRenderer/Animation/ImageAnimationRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/MovieRenderer/MovieRenderer.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Stage/Ui_StageSelect_StageUI.h"
#include "MapMarker/StageMapMarker.h"
#include "../cereal/include/cereal/types/vector.hpp"

namespace GamePlay::Ui
{
    class StageSelectUi final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IStartable
    {
    public:
        [[nodiscard]] std::vector<std::weak_ptr<StageSelectStageUi>> Stages() const;
        [[nodiscard]] rxcpp::observable<NanamiUi::MouseState> OnWorldEnterButtonClicked() const { return worldEnterButton_->OnClick(); }

        void HighlightSelectedStage(size_t selectedIndex);
        void SetWorldEnterButtonEnabled(bool isEnabled);
        Coroutine::Task<void> PlayEnterWorldTransitionAsync(GameCore::Scene::Main::SceneType sceneType);
        void ShowMapMarker(const glm::vec2& position, bool isCleared);

    private:
        void OnAwake() override;
        void OnStart() override;
        void OnDestroy() override;
        Coroutine::Task<void> StartStageSelectAsync();
        Coroutine::Task<void> AppearBackGroundMaskAsync();
        Coroutine::Task<void> FadeBlendRateAsync(std::shared_ptr<NanamiUi::BlendImageRenderer> renderer, int from, int to);
        Coroutine::Task<void> FadeBlendRateAsync(std::shared_ptr<NanamiUi::MovieRenderer> renderer, int from, int to);



        [[serialize(0)]] FIELD(Asset::SoundFile) bgm_;

        [[serialize(0)]] std::string backGroundMaskName_;
        FIELD(NanamiUi::BlendImageRenderer) backGroundMask_;
        [[serialize(0)]] int backGroundMaskBlendRate_ = 55;
        [[serialize(0)]] std::vector<std::string> stageSelectButtonNames_;
        std::vector<FIELD(StageSelectStageUi)> stageSelectButtons_;

        [[serialize(1)]] std::string stageSelectBackGroundMaskName_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) stageSelectBackGroundMask_;
        [[serialize(1)]] int stageSelectBackGroundMaskBlendRate_ = 50;
        [[serialize(1)]] std::string worldMovieRendererName_;
        FIELD(NanamiUi::MovieRenderer) worldMovieRenderer_;
        bool isEnteringWorld_ = false;
        [[serialize(1)]] std::string worldEnterButtonName_;
        FIELD(NanamiUi::Button) worldEnterButton_;
        FIELD(NanamiUi::ImageAnimationRenderer) worldEnterButtonGlow_;
        [[serialize(2)]] std::string backGroundName_;
        FIELD(NanamiUi::MovieRenderer) backGround_;
        [[serialize(3)]] FIELD(Asset::SpriteFile) worldEnterButtonActiveSprite_;
        [[serialize(3)]] FIELD(Asset::SpriteFile) worldEnterButtonDisabledSprite_;
        [[serialize(4)]] std::string mapMarkerName_;
        FIELD(StageMapMarker) mapMarker_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(bgm_));
            archive(CEREAL_NVP(backGroundMaskName_));
            archive(CEREAL_NVP(backGroundMaskBlendRate_));
            archive(CEREAL_NVP(stageSelectButtonNames_));
            archive(CEREAL_NVP(stageSelectBackGroundMaskName_));
            archive(CEREAL_NVP(stageSelectBackGroundMask_));
            archive(CEREAL_NVP(stageSelectBackGroundMaskBlendRate_));
            archive(CEREAL_NVP(worldMovieRendererName_));
            archive(CEREAL_NVP(worldEnterButtonName_));
            archive(CEREAL_NVP(backGroundName_));
            archive(CEREAL_NVP(worldEnterButtonActiveSprite_));
            archive(CEREAL_NVP(worldEnterButtonDisabledSprite_));
            archive(CEREAL_NVP(mapMarkerName_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bgm_));
            if (version >= 0) archive(CEREAL_NVP(backGroundMaskName_));
            if (version >= 0) archive(CEREAL_NVP(backGroundMaskBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(stageSelectButtonNames_));
            if (version >= 1) archive(CEREAL_NVP(stageSelectBackGroundMaskName_));
            if (version >= 1) archive(CEREAL_NVP(stageSelectBackGroundMask_));
            if (version >= 1) archive(CEREAL_NVP(stageSelectBackGroundMaskBlendRate_));
            if (version >= 1) archive(CEREAL_NVP(worldMovieRendererName_));
            if (version >= 1) archive(CEREAL_NVP(worldEnterButtonName_));
            if (version >= 2) archive(CEREAL_NVP(backGroundName_));
            if (version >= 3) archive(CEREAL_NVP(worldEnterButtonActiveSprite_));
            if (version >= 3) archive(CEREAL_NVP(worldEnterButtonDisabledSprite_));
            if (version >= 4) archive(CEREAL_NVP(mapMarkerName_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageSelectUi, 4)
