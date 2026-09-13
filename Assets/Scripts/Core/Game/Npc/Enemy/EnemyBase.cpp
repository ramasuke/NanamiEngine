#include "EnemyBase.h"

#include "../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../Editor/Npc/Enemy/Behaviour/Window/RunningEnemyBehaviourTreeWindow.h"
#include "../../../../GamePlay/Npc/Enemy/NetworkBehaviourTree/GamePlay_NetworkBehaviourTree.h"
#include "../../PlayerAvatar/LockOnTarget/LockOnPoint.h"
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

        for (const auto& child : Transform().GetAllChildren())
        {
            if (const auto lockOnPoint = child->Components().Catch<PlayerAvatar::LockOnPoint>().lock())
            {
                lockOnPoint_ = lockOnPoint;
                break;
            }
        }

        DoAwake();
    }

    glm::vec3 EnemyBase::LockOnPosition()
    {
        const auto lockOnPoint = lockOnPoint_.lock();
        if (!lockOnPoint)
            return Transform().GetWorldPos();

        // ツールで追加した子の worldMatrix_ は読み込み直後に古いことがあるため、ローカル行列を自分まで積み上げる
        const auto self = Entity().lock();
        glm::vec4 position(lockOnPoint->Transform().GetLocalPos(), 1.0f);
        for (auto parent = lockOnPoint->Transform().GetParent(); parent && parent != self; parent = parent->Transform().GetParent())
            position = parent->Transform().GetLocalMatrix() * position;

        return glm::vec3(Transform().GetWorldMatrix() * position);
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
                // ゲート内では isAuthorityGated == true ⇔ 自分が権威(他ピアはTickしていない)
                behaviour_->Tick(Entity(), currentStatus_, onDamagedStack_, GetNetworkObjectId(), isAuthorityGated);
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
