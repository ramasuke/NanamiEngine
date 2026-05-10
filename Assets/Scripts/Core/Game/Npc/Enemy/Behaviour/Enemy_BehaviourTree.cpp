#include "Enemy_BehaviourTree.h"

#include <fstream>

#include "../../../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../../../Editor/BehaviourTree/Window/Node/Entry/Npc_BehaviourEntryNode.h"
#include "../cereal/include/cereal/archives/json.hpp"

namespace GameCore::Npc::Enemy
{
    BehaviourTree::BehaviourTree(std::string filePath)
        : filePath_  (std::move(filePath))
        , entryNode_ (std::make_unique<Editor::Npc::Behaviour::EntryNode>())
        , parameters_(std::make_unique<BlackBoard::ParameterGroup>())
    {
        std::ifstream ifStream(filePath_);
        if (!ifStream.is_open())
            return;

        cereal::JSONInputArchive archive(ifStream);
        archive(cereal::make_nvp("entryNode_", entryNode_));
        archive(cereal::make_nvp("parameters_", parameters_));
    }
    BehaviourTree::~BehaviourTree() = default;

    void BehaviourTree::Tick(const std::weak_ptr<GameObject::IGameObject>& enemyGameObject,
                             const std::unique_ptr<EnemyStatus>& enemyStatus,
                             const std::shared_ptr<std::queue<std::unique_ptr<IDamage>>>& onDamagedStack) const
    {
        entryNode_->Tick(Behaviour::Action::TickContext(enemyGameObject, enemyStatus, parameters_, onDamagedStack));
    }
    
    void BehaviourTree::OnSave()
    {
        std::ofstream ofStream(filePath_);
        if (!ofStream.is_open())
            return;
    
        cereal::JSONOutputArchive archive(ofStream);

        archive(cereal::make_nvp("entryNode_", entryNode_));
        archive(cereal::make_nvp("parameters_", parameters_));
    }

    void BehaviourTree::OnDrawGraphEditorGui()
    {
        Editor::Npc::Behaviour::NodeFactory::Instance().ChangeBehaviourTreeType(BehaviourTreeType::EnemyNpc);
        ImGui::Begin(("BehaviourTree##" + guid_.Value()).c_str(), nullptr);
        ImDrawList*  drawList   = ImGui::GetWindowDrawList();
        const ImVec2 offset     = ImGui::GetCursorScreenPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        Gui::Graph::DrawGrid(drawList, offset, windowSize, K_GRID_STEP,K_GRID_COLOR);

        entryNode_->OnDrawGraphEditorGui(offset, drawList, entryNode_);
        ImGui::End();
    }

    void BehaviourTree::OnDrawGui()
    {
        ImGui::Begin(filePath_.c_str());
        parameters_->OnDrawGui();
        ImGui::End();
    }
}
