#include "EnemyBase.h"

#include "../../../../../../Engine/Module/Component/Animator/Animator.h"
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
        
        DoAwake();
    }

    void EnemyBase::OnUpdate()
    {
        currentStatus_->Get().ManualUpdate();
        if (behaviour_)
        {
            behaviour_->Tick(Entity(), currentStatus_, onDamagedStack_);
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
    }
}
