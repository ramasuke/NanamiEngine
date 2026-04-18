#include "EnemyBase.h"

#include "../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "Behaviour/Enemy_BehaviourTree.h"

namespace GameCore::Npc
{
    EnemyBase::EnemyBase()
        : status_(std::make_unique<Enemy::EnemyStatus>(1))
        , onDamagedStack_(std::make_shared<std::queue<std::unique_ptr<IDamageContext>>>())
    {
        
    }

    EnemyBase::~EnemyBase() = default;

    void EnemyBase::OnAwake()
    {
        RequireComponent<Component::Animator>();

        if (behaviourData_)
        {
            behaviour_ = behaviourData_->OnLoadCopyContent();
        }
        DoAwake();
    }

    void EnemyBase::OnUpdate()
    {
        status_->ManualUpdate();
        if (behaviour_)
        {
            behaviour_->Tick(Entity(), status_, onDamagedStack_);
        }
        DoUpdate();
    }

    void EnemyBase::OnTakeDamage(std::unique_ptr<IDamageContext> context)
    {
        onDamagedStack_->push(std::move(context));
    }

    void EnemyBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("behaviourData_", behaviourData_);
        ImGuiHelper::OnDrawInputField("status_", status_) ;
    }
}
