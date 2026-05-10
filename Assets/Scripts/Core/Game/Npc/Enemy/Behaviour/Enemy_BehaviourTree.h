#pragma once
#include "../../../../../../../Engine/Core/Object/IObject.h"
#include "Action/TickContext/Enemy_Behaviour_TickContext.h"

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace Editor::Npc::Behaviour
{
    class EntryNode;
}

namespace GameCore::Npc::Enemy
{
    static constexpr float K_GRID_STEP = 64.0f;
    static constexpr ImU32 K_GRID_COLOR = IM_COL32(60, 60, 60, 255);
    
    class BehaviourTree final : public Object::IObject
    {
    public:
        explicit BehaviourTree(std::string filePath = "");
        ~BehaviourTree() override;

        void Tick(const std::weak_ptr<GameObject::IGameObject>& enemyGameObject,
                  const std::unique_ptr<EnemyStatus>& enemyStatus,
                  const std::shared_ptr<std::queue<std::unique_ptr<IDamage>>>& onDamagedStack) const;
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
