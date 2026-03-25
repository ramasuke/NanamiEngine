#pragma once
#include "../../../../../../../Engine/Core/Application/Window/Main/MainWindowBase.h"
#include "../../../../../../../Engine/Core/Application/Window/Main/Factory/MainWindowFactory.h"
#include "../../../../../Core/Game/Npc/Enemy/Behaviour/Enemy_BehaviourTree.h"

namespace Editor::Npc::Enemy
{
    class EnemyNpcBehaviourWindow final : public Core::MainWindow::MainWindowBase<GameCore::Npc::Enemy::BehaviourTree>
    {
        void OnSave() override;
        void OnDrawGui(Core::MainWindow::MainWindowDrawGuiContext context) override;
        void OnUpdate() override;
    };
    
    REGISTER_MAIN_WINDOW(EnemyNpcBehaviourWindow);
}
