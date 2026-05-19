#pragma once
#include "../../../../../../../Engine/Core/Application/Window/Main/MainWindowBase.h"
#include "../../../../../../../Engine/Core/Application/Window/Main/Factory/MainWindowFactory.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/Friendly_BehaviourTree.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace Editor::Npc::Friendly
{
    class FriendlyNpcBehaviourWindow final : public Core::MainWindow::MainWindowBase<GameCore::Npc::Friendly::BehaviourTree>
    {
    public:
        explicit FriendlyNpcBehaviourWindow();

    private:
        void OnSave() override;
        void OnDrawGui(Core::MainWindow::MainWindowDrawGuiContext context) override;
        void OnUpdate() override;
    };
    
    REGISTER_MAIN_WINDOW(FriendlyNpcBehaviourWindow);
}
