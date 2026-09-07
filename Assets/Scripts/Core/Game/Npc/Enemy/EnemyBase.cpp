#include "EnemyBase.h"

#include "../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../Editor/Npc/Enemy/Behaviour/Window/RunningEnemyBehaviourTreeWindow.h"
#include "../../../../GamePlay/Npc/Enemy/NetworkBehaviourTree/GamePlay_NetworkBehaviourTree.h"
#include "Behaviour/Enemy_BehaviourTree.h"

namespace GameCore::Npc
{
    EnemyBase::EnemyBase()
        : onDamagedStack_(std::make_shared<std::queue<std::unique_ptr<IDamage>>>())
    {
        
    }

    EnemyBase::~EnemyBase() = default;

    void EnemyBase::OnAwake()
    {
        RequireComponent<Component::Animator>();

        if (behaviourData_)
            behaviour_ = behaviourData_->OnLoadCopyContent();

        hasNetworkBehaviourTree_ = Components().Catch<GamePlay::Npc::Enemy::NetworkBehaviourTree>().lock() != nullptr;

        DoAwake();
    }

    void EnemyBase::OnUpdate()
    {
        // NetworkBehaviourTree が付与されており、かつ有効な NetworkObjectId を持つ個体だけ権威側限定でTickする。
        // まだ有効なIDを持たない個体(スポーン経路未対応)は従来通り全ピアでローカルTickし続ける。
        const bool isAuthorityGated = hasNetworkBehaviourTree_
            && GetNetworkObjectId() != NanamiEngine::Core::Network::NetworkObjectId::Invalid();

        if (!isAuthorityGated || HasStateAuthority())
        {
            currentStatus_->Get().ManualUpdate();
            if (behaviour_)
            {
                behaviour_->Tick(Entity(), currentStatus_, onDamagedStack_);
            }
        }
        DoUpdate();
    }

    void EnemyBase::OnTakeDamage(std::unique_ptr<IDamage> context)
    {
        onDamagedStack_->push(std::move(context));
    }

    void EnemyBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("behaviourData_", behaviourData_);
        ImGuiHelper::OnDrawInputField("currentStatus_", currentStatus_);
        if (ImGui::Button("CreateCurrentStatus"))
        {
            currentStatus_ = CreateSyncParameter(Enemy::EnemyStatus());
        }
        ImGuiHelper::OnDrawInputField("isNetworkSyncStatus_", isNetworkSyncStatus_);

        if (behaviour_ && ImGui::Button("Show Running BehaviourTree"))
        {
            for (auto* window : Core::Application::ApplicationBase::PopupWindows().Catch<Editor::Npc::Enemy::RunningEnemyBehaviourTreeWindow>())
                window->TryAddTarget(behaviour_);
        }
    }
}
