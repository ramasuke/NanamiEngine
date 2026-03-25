#include "Data_FriendNpcBehaviourFile.h"

#include "../../Scripts/Core/Game/Npc/Friendly/Behaviour/Friendly_BehaviourTree.h"
#include "../../Scripts/Editor/Npc/Friendly/Behaviour/Window/FriendlyNpcBehaviourWindow.h"

namespace NanamiEngine::Module::Asset
{
    FriendNpcBehaviourFile::FriendNpcBehaviourFile(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::unique_ptr<GameCore::Npc::Friendly::BehaviourTree> FriendNpcBehaviourFile::OnLoadCopyContent() const
    {
        return std::make_unique<GameCore::Npc::Friendly::BehaviourTree>(GetContentPath()); 
    }

    void FriendNpcBehaviourFile::OnDoubleClick()
    {
        Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Friendly::FriendlyNpcBehaviourWindow>());
        Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Friendly::FriendlyNpcBehaviourWindow>()->AddContent(OnLoadCopyContent());
    }

    void FriendNpcBehaviourFile::OnSaveCallback()
    {
        std::make_shared<GameCore::Npc::Friendly::BehaviourTree>(GetContentPath())->OnSave();
    }
}
