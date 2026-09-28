#pragma once
#include "Engine/Core/Object/IObject.h"
#include "Action/TickContext/Friendly_Behaviour_TickContext.h"

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace NanamiEngine::Module::Gui::Graph
{
    class GraphEditorHost;
}

namespace Editor::Npc::Behaviour
{
    class EntryNode;
    class NodeBase;
    class BehaviourTreeGraphDelegate;
}

namespace GameCore::Npc::Friendly
{
    class BehaviourTree final : public Object::IObject
    {
    public:
        explicit BehaviourTree(std::string filePath = "");
        ~BehaviourTree() override;

        void Tick(const std::string& npcName,
                  const std::weak_ptr<GameObject::IGameObject>& ownGameObject,
                  const std::weak_ptr<GamePlay::Ui::BillBoardNpcChatIcon>& ownChatIcon,
                  bool& isChatting) const;
        void OnSave();
        void OnDrawGraphEditorGui(bool readOnly = false);
        void OnDrawGui() override;
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        [[nodiscard]] const std::string& GetFilePath() const { return filePath_; }

    private:
        std::string filePath_;
        Guid guid_;

        std::shared_ptr<Editor::Npc::Behaviour::EntryNode> entryNode_;
        std::vector<std::shared_ptr<Editor::Npc::Behaviour::NodeBase>> detachedNodes_;
        std::unique_ptr<BlackBoard::ParameterGroup> parameters_;

        std::shared_ptr<NanamiEngine::Module::Gui::Graph::GraphEditorHost> graphHost_;
        std::shared_ptr<Editor::Npc::Behaviour::BehaviourTreeGraphDelegate> graphDelegate_;
    };
}
