#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../Model/StageSelectModel.h"

namespace GamePlay::Ui
{
    class StageSelectUi;
}

namespace GamePlay::Ui
{
    class StageSelectPresenter final : public Component::ComponentBase,
                                       public LifeCycleCallback::IStartable,
                                       public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart() override;
        void OnUpdate() override;
        void TryEnterWorld();

        std::shared_ptr<StageSelectUi> view_;
        std::unique_ptr<StageSelectModel> model_;
        bool wasConfirmPressed_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override {}

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageSelectPresenter, 0)
