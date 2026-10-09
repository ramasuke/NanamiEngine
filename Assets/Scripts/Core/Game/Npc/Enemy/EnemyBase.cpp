#include "EnemyBase.h"

#include <algorithm>

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../Editor/Npc/Enemy/Behaviour/Window/RunningEnemyBehaviourTreeWindow.h"
#include "../../../../GamePlay/PlayerAvatar/HitShakeReceiver/PlayerHitShakeReceiver.h"
#include "../../../../GamePlay/Npc/Enemy/NetworkBehaviourTree/GamePlay_NetworkBehaviourTree.h"
#include "../../../../GamePlay/Pickup/GamePlay_LootDrop.h"
#include "Behaviour/Enemy_BehaviourTree.h"
#include "../../PlayerAvatar/Record/PlayerAvatar_RecordBook.h"
#include "../../../Network/Rpc/Custom_RpcType.h"
#include "ShowHealthGaugeProvider/IShowHealthGaugeProvider.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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
        RequireComponent<GamePlay::PlayerAvatar::PlayerHitShakeReceiver>();

        if (behaviourData_)
            behaviour_ = behaviourData_->OnLoadCopyContent();

        hasNetworkBehaviourTree_ = Components().Catch<GamePlay::Npc::Enemy::NetworkBehaviourTree>().lock() != nullptr;
        showHealthGaugeProvider_ = dynamic_cast<Enemy::IShowHealthGaugeProvider*>(this);

        DoAwake();
    }

    glm::vec3 EnemyBase::LockOnPosition()
    {
        const auto lockOnPoint = lockOnPoint_.get();
        if (!lockOnPoint)
            return Transform().GetWorldPos();

        // NOTE: ツールで追加した子の worldMatrix_ は読み込み直後に古いことがあるため、ローカル行列を自分まで積み上げる
        const auto self = Entity().lock();
        glm::vec4 position(lockOnPoint->Transform().GetLocalPos(), 1.0f);
        for (auto parent = lockOnPoint->Transform().GetParent(); parent && parent != self; parent = parent->Transform().GetParent())
            position = parent->Transform().GetLocalMatrix() * position;

        return glm::vec3(Transform().GetWorldMatrix() * position);
    }

    void EnemyBase::NotifyDefeated()
    {
        if (isDefeatRecorded_)
            return;
        
        isDefeatRecorded_ = true;
        if (const auto kind = RecordKind())
            PlayerAvatar::Record::RecordBook::Instance().RecordDefeat(*kind);

        // NOTE: 全ピアで呼ばれ、拾い物はその PC のプレイヤーへ飛ぶので、協力プレイでは各自が全額を受け取る
        if (dropTable_)
            GamePlay::Pickup::DropLoot(*dropTable_.get(), LockOnPosition());
    }

    void EnemyBase::OnUpdate()
    {
        // NOTE: NetworkBehaviourTree 付きで有効な NetworkObjectId を持つ個体だけ、権威側限定で Tick する
        const bool isAuthorityGated = hasNetworkBehaviourTree_
            && GetNetworkObjectId() != NanamiEngine::Core::Network::NetworkObjectId::Invalid();

        if (!isAuthorityGated || HasStateAuthority())
        {
            currentStatus_->Get().ManualUpdate();
            if (behaviour_)
            {
                // NOTE: ここでは isAuthorityGated が true なら自分が権威 (他ピアは Tick していない)
                behaviour_->Tick(Entity(), currentStatus_, onDamagedStack_, showHealthGaugeProvider_, GetNetworkObjectId(), isAuthorityGated,
                                pendingFlinchPower_);
                // NOTE: Flinch を持たないツリーや届かない枝で、古い怯みが後から効かないよう毎 Tick 捨てる
                pendingFlinchPower_.reset();
            }
        }
        SendHealthIfChanged();
        DoUpdate();
    }

    void EnemyBase::SendHealthIfChanged()
    {
        // NOTE: NetworkBehaviourTree の無い敵も BT は全ピアで回るが、ダメージはホストにしか入らないので HP はホストから配る
        if (GetNetworkObjectId() == NanamiEngine::Core::Network::NetworkObjectId::Invalid() || !HasStateAuthority())
            return;

        const int health = currentStatus_->Get().Health().Value();
        if (lastSentHealth_ == health)
            return;

        lastSentHealth_ = health;
        GameCore::Network::EnemyHealthRpc::Send(GetNetworkObjectId(), Core::Network::DeliveryMode::Reliable, health);
    }

    void EnemyBase::ApplyNetworkHealth(const int value)
    {
        currentStatus_->Get().ApplyNetworkHealth(value);
    }

    void EnemyBase::OnTakeDamage(std::unique_ptr<IDamage> context)
    {
        const Damage::FlinchPower flinchPower = context->FlinchPower();
        pendingFlinchPower_ = pendingFlinchPower_ ? std::max(*pendingFlinchPower_, flinchPower) : flinchPower;
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
        ImGuiHelper::OnDrawInputField("lockOnPoint_", lockOnPoint_);
        ImGuiHelper::OnDrawInputField("dropTable_", dropTable_);

        if (behaviour_ && ImGui::Button("Show Running BehaviourTree"))
        {
            for (auto* window : Core::Application::ApplicationBase::PopupWindows().Catch<Editor::Npc::Enemy::RunningEnemyBehaviourTreeWindow>())
                window->TryAddTarget(behaviour_);
        }
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Npc::EnemyBase);
#pragma endregion
