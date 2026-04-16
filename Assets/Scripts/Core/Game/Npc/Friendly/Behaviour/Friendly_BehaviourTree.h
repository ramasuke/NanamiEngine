#pragma once
#include "../../../../../../../Engine/Core/Object/IObject.h"
#include "Action/TickContext/Friendly_Behaviour_TickContext.h"

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace Editor::Npc::Behaviour
{
    class EntryNode;
}

namespace GameCore::Npc::Friendly
{
    static constexpr float K_GRID_STEP = 64.0f;
    static constexpr ImU32 K_GRID_COLOR = IM_COL32(60, 60, 60, 255);
    
    class BehaviourTree final : public Object::IObject
    {
    public:
        explicit BehaviourTree(std::string filePath = "");
        ~BehaviourTree() override;

        void Tick(const std::string& npcName,
                  const std::weak_ptr<GameObject::IGameObject>& ownGameObject,
                  bool& isChatting) const;
        void OnSave();
        void OnDrawGraphEditorGui();
        void OnDrawGui() override;
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        
    private:
        std::string filePath_;
        Guid guid_;

        std::shared_ptr<Editor::Npc::Behaviour::EntryNode> entryNode_;
        std::unique_ptr<BlackBoard::ParameterGroup> parameters_;  
    };
}
