#include "Data_EnemyBehaviourFile.h"

#include "../../../Engine/Module/Exception/Engine_Module_Exception.h"
#include "../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
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
        try
        {
            auto behaviourTree = std::make_unique<GameCore::Npc::Enemy::BehaviourTree>(GetContentPath());
            Core::Application::ApplicationBase::ApplicationLifeCycle().OnUpdateFieldInittables();
            return behaviourTree;
        }
        catch (const Exception::SerializationException& exception)
        {
            LogError("EnemyBehaviourFile: " + std::string(exception.what()));
            return nullptr;
        }
    }

    void EnemyBehaviourFile::OnDoubleClick()
    {
        auto behaviourTree = OnLoadCopyContent();
        if (!behaviourTree)
            return;

        Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Enemy::EnemyNpcBehaviourWindow>());
        Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Enemy::EnemyNpcBehaviourWindow>()->AddContent(std::move(behaviourTree));
    }

    void EnemyBehaviourFile::OnSaveCallback()
    {
        std::make_shared<GameCore::Npc::Enemy::BehaviourTree>(GetContentPath())->OnSave();
    }
}
