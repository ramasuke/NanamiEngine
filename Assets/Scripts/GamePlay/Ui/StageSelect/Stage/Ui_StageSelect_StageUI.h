#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "../../../../Core/Game/Scene/Main/Type/MainSceneType.h"

namespace GamePlay::Ui
{
    class StageSelectStageUi final : public Component::ComponentBase,
                                     public LifeCycleCallback::IAwakable
    {
    public:
        void SubscribeOnClickSelectButton(std::function<void()> onClick);
        [[nodiscard]] GameCore::Scene::Main::SceneType SceneType() const { return stageSceneType_; }
        
        
    private:
        void OnAwake() override;

    private:
        FIELD(NanamiUi::Button) selectButton_;
        [[serialize(0)]] FIELD(Asset::SoundFile) selectButtonHoverSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) selectButtonClickSound_;
        [[serialize(0)]] GameCore::Scene::Main::SceneType stageSceneType_ = GameCore::Scene::Main::SceneType::GrassLand;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(selectButtonHoverSound_));
            archive(CEREAL_NVP(selectButtonClickSound_));
            archive(CEREAL_NVP(stageSceneType_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(selectButtonHoverSound_));
            if (version >= 0) archive(CEREAL_NVP(selectButtonClickSound_));
            if (version >= 0) archive(CEREAL_NVP(stageSceneType_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectStageUi, 0);
CEREAL_REGISTER_TYPE(GamePlay::Ui::StageSelectStageUi);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Ui::StageSelectStageUi);
#pragma endregion
