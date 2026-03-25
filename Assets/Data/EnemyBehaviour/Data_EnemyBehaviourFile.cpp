#include "Data_EnemyBehaviourFile.h"

#include "../../Scripts/Core/Game/Npc/Enemy/Behaviour/Enemy_BehaviourTree.h"
#include "../../Scripts/Editor/Npc/Enemy/Behaviour/Window/EnemyNpcBehaviourWindow.h"

namespace NanamiEngine::Module::Asset
{
    EnemyBehaviourFile::EnemyBehaviourFile(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::unique_ptr<GameCore::Npc::Enemy::BehaviourTree> EnemyBehaviourFile::OnLoadCopyContent() const
    {
        auto behaviourTree = std::make_unique<GameCore::Npc::Enemy::BehaviourTree>(GetContentPath());
        Core::Application::ApplicationBase::ApplicationLifeCycle().OnUpdateFieldInittables();
        return std::move(behaviourTree);
    }

    void EnemyBehaviourFile::OnDoubleClick()
    {
        Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Enemy::EnemyNpcBehaviourWindow>());
        Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Enemy::EnemyNpcBehaviourWindow>()->AddContent(OnLoadCopyContent());
    }

    void EnemyBehaviourFile::OnSaveCallback()
    {
        std::make_shared<GameCore::Npc::Enemy::BehaviourTree>(GetContentPath())->OnSave();
    }
}
