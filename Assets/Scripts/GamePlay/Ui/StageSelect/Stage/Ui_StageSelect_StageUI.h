#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"

namespace GamePlay::Ui
{
    class StageSelectStageUi final : public Component::ComponentBase,
                                     public LifeCycleCallback::IAwakable,
                                     public LifeCycleCallback::IStartable
    {
    public:
        
        
    private:
        void OnAwake() override;
        void OnStart() override;

    private:
        [[serialize(0)]] std::string selectButtonName_;
        FIELD(NanamiUi::Button) selectButton_;
        [[serialize(0)]] std::string goTripButtonName_;
        FIELD(NanamiUi::Button) goTripButton_;
    };
}
