#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"

namespace GamePlay::Ui
{
    class SampleTitleScene final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable,
                                   public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart() override;
        void OnUpdate() override;

        
        [[serialize(0)]] FIELD(NanamiUi::Button) gameStartButton_;
        [[serialize(0)]] FIELD(NanamiUi::Button) gameExitButton_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(gameStartButton_));
            archive(CEREAL_NVP(gameExitButton_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(gameStartButton_));
            if (version >= 0) archive(CEREAL_NVP(gameExitButton_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::SampleTitleScene, 0);
CEREAL_REGISTER_TYPE(GamePlay::Ui::SampleTitleScene);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Ui::SampleTitleScene);
#pragma endregion