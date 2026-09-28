#include "Enemy_BehaviourTree.h"

#include <fstream>

#include "Engine/Core/Network/Object/Creator/NetworkParamCreator.h"
#include "Engine/Module/Gui/Graph/Editor/GraphEditorHost.h"
#include "Engine/Module/Serialization/Engine_Module_Serialization.h"
#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "../../../../../Editor/BehaviourTree/Window/Node/Entry/Npc_BehaviourEntryNode.h"
#include "../../../../../Editor/BehaviourTree/Window/Graph/BehaviourTreeGraphDelegate.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "../cereal/include/cereal/types/vector.hpp"

namespace GameCore::Npc::Enemy
{
    BehaviourTree::BehaviourTree(std::string filePath)
        : filePath_  (std::move(filePath))
        , entryNode_ (std::make_unique<Editor::Npc::Behaviour::EntryNode>())
        , parameters_(std::make_unique<BlackBoard::ParameterGroup>())
    {
        NanamiEngine::Module::Serialization::LoadJsonFileIfExists(filePath_, [this](cereal::JSONInputArchive& archive)
        {
            archive(cereal::make_nvp("entryNode_", entryNode_));
            archive(cereal::make_nvp("parameters_", parameters_));
            try
            {
                archive(cereal::make_nvp("detachedNodes_", detachedNodes_));
            }
            catch (const cereal::Exception&)
            {
                detachedNodes_.clear();
            }
        });
    }
    BehaviourTree::~BehaviourTree() = default;

    void BehaviourTree::Tick(
        const std::weak_ptr<GameObject::IGameObject>& enemyGameObject,
        SyncParam<EnemyStatus>& enemyStatus,
        const std::shared_ptr<std::queue<std::unique_ptr<IDamage>>>& onDamagedStack,
        IShowHealthGaugeProvider* const showHealthGaugeProvider,
        const Core::Network::NetworkObjectId networkObjectId,
        const bool isNetworkAuthority,
        std::optional<Damage::FlinchPower>& pendingFlinchPower) const
    {
        ++tickIndex_;
        entryNode_->Tick(Behaviour::Action::TickContext(
            enemyGameObject, enemyStatus, parameters_, onDamagedStack, showHealthGaugeProvider, networkObjectId, isNetworkAuthority,
            pendingFlinchPower, tickIndex_));
    }
    
    void BehaviourTree::OnSave()
    {
        std::ofstream ofStream(filePath_);
        if (!ofStream.is_open())
            return;
    
        cereal::JSONOutputArchive archive(ofStream);

        archive(cereal::make_nvp("entryNode_", entryNode_));
        archive(cereal::make_nvp("parameters_", parameters_));
        if (!detachedNodes_.empty())
            archive(cereal::make_nvp("detachedNodes_", detachedNodes_));
    }

    void BehaviourTree::OnDrawGraphEditorGui(const bool readOnly)
    {
        Editor::Npc::Behaviour::NodeFactory::Instance().ChangeBehaviourTreeType(BehaviourTreeType::EnemyNpc);

        if (!graphHost_)     graphHost_     = std::make_shared<NanamiEngine::Module::Gui::Graph::GraphEditorHost>();
        if (!graphDelegate_) graphDelegate_ = std::make_shared<Editor::Npc::Behaviour::BehaviourTreeGraphDelegate>();

        const auto        separator = filePath_.find_last_of("/\\");
        const std::string fileName  = separator == std::string::npos ? filePath_ : filePath_.substr(separator + 1);
        const std::string title = std::string(readOnly ? "BehaviourTree [Running] " : "BehaviourTree ") + fileName + "##" + guid_.Value();
        if (ImGui::Begin(title.c_str(), nullptr))
            graphDelegate_->Draw(entryNode_, detachedNodes_, *graphHost_, readOnly);
        ImGui::End();
    }

    void BehaviourTree::OnDrawGui()
    {
        ImGui::Begin(filePath_.c_str());
        parameters_->OnDrawGui();
        ImGui::End();
    }
}
