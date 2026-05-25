#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Stage/Ui_StageSelect_StageUI.h"

namespace GamePlay::Ui
{
    class StageSelectUi final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IStartable
    {

    private:
        void OnAwake() override;
        void OnStart() override;
        Coroutine::Task<void> StartStageSelectAsync();
        Coroutine::Task<void> AppearBackGroundMaskAsync();
        

        [[serialize(0)]] std::string backGroundMaskName_;
        FIELD(NanamiUi::BlendImageRenderer) backGroundMask_;
        [[serialize(0)]] int backGroundMaskBlendRate_ = 55;
        [[serialize(0)]] std::vector<std::string> stageSelectButtonNames_;
        std::vector<FIELD(StageSelectStageUi)> stageSelectButtons_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(backGroundMaskName_));
            archive(CEREAL_NVP(backGroundMaskBlendRate_));
            //archive(CEREAL_NVP(stageSelectButtonNames_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(backGroundMaskName_));
            if (version >= 0) archive(CEREAL_NVP(backGroundMaskBlendRate_));
            //if (version >= 0) archive(CEREAL_NVP(stageSelectButtonNames_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectUi, 0);
CEREAL_REGISTER_TYPE(GamePlay::Ui::StageSelectUi);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Ui::StageSelectUi);
#pragma endregion