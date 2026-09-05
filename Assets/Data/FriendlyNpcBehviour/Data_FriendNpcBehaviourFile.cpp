#include "Data_FriendNpcBehaviourFile.h"

#include "../../../Engine/Module/Exception/Engine_Module_Exception.h"
#include "../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
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
        try
        {
            return std::make_unique<GameCore::Npc::Friendly::BehaviourTree>(GetContentPath());
        }
        catch (const Exception::SerializationException& exception)
        {
            // 壊れた BehaviourTree は「無い」ものとして扱う。FriendlyNpc 側は behaviour_ の null チェックで動作を続ける
            LogError("FriendNpcBehaviourFile: " + std::string(exception.what()));
            return nullptr;
        }
    }

    void FriendNpcBehaviourFile::OnDoubleClick()
    {
        auto behaviourTree = OnLoadCopyContent();
        if (!behaviourTree)
            return;

        Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Friendly::FriendlyNpcBehaviourWindow>());
        Core::Application::ApplicationBase::MainWindows().Catch<Editor::Npc::Friendly::FriendlyNpcBehaviourWindow>()->AddContent(std::move(behaviourTree));
    }

    void FriendNpcBehaviourFile::OnSaveCallback()
    {
        std::make_shared<GameCore::Npc::Friendly::BehaviourTree>(GetContentPath())->OnSave();
    }
}
