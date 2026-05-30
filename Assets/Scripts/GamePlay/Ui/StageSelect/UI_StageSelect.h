#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/MovieRenderer/MovieRenderer.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Stage/Ui_StageSelect_StageUI.h"
#include "../cereal/include/cereal/types/vector.hpp"

namespace GamePlay::Ui
{
    class StageSelectUi final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IStartable
    {

    private:
        void OnAwake() override;
        void OnStart() override;
        void OnDestroy() override;
        Coroutine::Task<void> StartStageSelectAsync();
        Coroutine::Task<void> AppearBackGroundMaskAsync();
        

        
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
        bool hasSelectedSceneType_ = false;
        GameCore::Scene::Main::SceneType selectedSceneType_ = GameCore::Scene::Main::SceneType::GrassLand;
        [[serialize(1)]] std::string worldEnterButtonName_;
        FIELD(NanamiUi::Button) worldEnterButton_;
        
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
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectUi, 1);
CEREAL_REGISTER_TYPE(GamePlay::Ui::StageSelectUi);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Ui::StageSelectUi);
#pragma endregion
