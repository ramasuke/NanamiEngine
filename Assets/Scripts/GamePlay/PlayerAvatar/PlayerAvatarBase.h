#pragma once
#include <utility>

#include "../../Core/Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "Engine/Module/LifeCycleCallback/FixedUpdate/IFixedUpdatable.h"
#include "Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../Core/Game/Damage/Game_Damage_IDamage.h"
#include "../../Core/Game/Npc/Enemy/ITakableEnemyAttack/ITakableEnemyAttack.h"
#include "../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../Core/Game/PlayerAvatar/StateMachine/PlayerAvatarStateMachineBase.h"
#include "../../Core/Game/PlayerAvatar/RequireType/RequireType.h"
#include "../../Core/Game/PlayerAvatar/Status/PlayerAvatarStatus.h"
#include "../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_ITakeableQuest.h"
#include "../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "../Ui/NpcChatting/Ui_NpcChatting.h"
#include "InteractableArea/InteractableArea.h"
#include "WakeUpArea/WakeUpArea.h"
#include "../../Core/Game/PlayerAvatar/Wakeable/IPlayerWakeable.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/IVirtualCameraTarget.h"

namespace GamePlay::PlayerAvatar
{
    using namespace GameCore::PlayerAvatar;
    template <RequireType::Traits TraitsT>
    class PlayerAvatarBase : public Module::Network::NetworkComponent,
                             public LifeCycleCallback::IAwakable,
                             public LifeCycleCallback::IUpdatable,
                             public LifeCycleCallback::IFixedUpdatable,
                             public GameCore::IPlayerAvatar,
                             public GameCore::Npc::Enemy::ITakableEnemyAttack,
                             public GameCore::PlayerAvatar::IPlayerWakeable,
                             public CineMachine::IVirtualCameraTarget
    {
        using Animator     = RequireType::Animator    <TraitsT>;
        using StateMachine = RequireType::StateMachine<TraitsT>;
        using State        = RequireType::State       <TraitsT>;
        using Status       = RequireType::Status      <TraitsT>;
        using InputAction  = RequireType::InputAction <TraitsT>;
        using CameraGroup  = RequireType::CameraGroup <TraitsT>;
        
    public:
        virtual ~PlayerAvatarBase() override;
        void Init(std::shared_ptr<Status      > status      ,
                  std::unique_ptr<StateMachine> stateMachine,
                  std::shared_ptr<InputAction > inputAction ,
                  const std::weak_ptr<CameraGroup>& cameraGroup,
                  bool isOwner);

        [[nodiscard]] bool IsOwner() const override { return isOwner_; }
        [[nodiscard]] IPlayerAvatarEventSceneStateMachine& GetEventSceneStateMachine() const override { return *stateMachine_; }
        [[nodiscard]] const StateMachine& GetStateMachine() const { return *stateMachine_; }
        [[nodiscard]] const InputAction& GetInputAction() const { return *inputAction_; }
        /** @brief PlayerAvatar<T>のCameraを取得 */
        [[nodiscard]] Component::RigidBody& RigidBody() const override { return *rigidBody_.lock(); }
        [[nodiscard]] GameObject::Transform& PlayerTransform() const override { return Transform(); }
        [[nodiscard]] Status& PlayerStatus() const override { return *status_; }
        void SaveStatus() override;
        /**
         * @brief 職業を問わないクエスト(QuestJournal)をこのアバターのものにする。手元で操作するアバターだけが呼ぶ。
         * 保存済みの状態から読み直すので、ステータスを読み込んだ直後に呼ぶ
         */
        void BindQuestJournal();
        void EnableStateMachiine() override;
        void DisableStateMachine() override;

    protected:
        [[nodiscard]] std::weak_ptr<CameraGroup> AvatarCameraGroup() const { return cameraGroup_; }

    private:
        void OnAwake                 () override;
        void OnUpdate                () override;
        void NetworkedTick           () override;
        void OnFixedUpdate           () override;
        void OnDestroy               () override;
        void BasedOnDrawgui          () override;
        void SubscribeStateToAnimator();
        void OnTakeDamage(std::unique_ptr<GameCore::IDamage> context) override
        {
            status_->AddOnDamageStack(std::move(context));
        }
        void ApplySyncState(uint8_t stateValue) override;

        [[nodiscard]] Ui::NpcChatting            & NpcChattingUi   () const override { return *chattingUi_.get(); }
        [[nodiscard]] PlayerAvatar::InteractableArea& InteractableArea   () const override;
        [[nodiscard]] PlayerAvatar::WakeUpArea   & WakeUpArea      () const override;
        [[nodiscard]] const glm::vec3            & FeatStepPosition() const override;

        void OnEnterWakeUpRange() override {}
        void OnExitWakeUpRange () override {}
        void RequestWakeUp     () override;
        [[nodiscard]] bool IsDowned() const override { return status_->IsDowned(); }
        [[nodiscard]] const GameObject::Transform& WakeableTransform() const override { return Transform(); }
        // ModelRendererは物理ステップ間を補間した位置に描くので、Transformを追うとカメラとモデルがずれてカクつく
        [[nodiscard]] glm::vec3 CameraTargetPosition() const override;

        std::weak_ptr<Component::Animator> animatorComponent_;
        std::weak_ptr<Component::ModelRenderer> modelRenderer_;

        std::unique_ptr<Animator          > animator_     = nullptr;
        std::unique_ptr<StateMachine      > stateMachine_ = nullptr;
        std::shared_ptr<State             > animatedState_ = nullptr;
        std::shared_ptr<Status            > status_       = nullptr;
        std::shared_ptr<InputAction       > inputAction_  = nullptr;
        std::weak_ptr  <CameraGroup       > cameraGroup_;
        std::weak_ptr  <Component::RigidBody> rigidBody_  ;
        bool isOwner_ = false;
        [[serialize(0)]] FIELD(Ui::NpcChatting) chattingUi_;
        [[serialize(3)]] FIELD(GameObject::IGameObject) featStep_;
        [[serialize(4)]] FIELD(PlayerAvatar::InteractableArea) interactableArea_;
        [[serialize(4)]] FIELD(PlayerAvatar::WakeUpArea) wakeUpArea_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            if (version <= 1)
                archive(cereal::base_class<ComponentBase>(this));
            else if (version >= 2)
                archive(cereal::base_class<NetworkComponent>(this));
            archive(CEREAL_NVP(chattingUi_));
            archive(CEREAL_NVP(featStep_));
            archive(CEREAL_NVP(interactableArea_));
            archive(CEREAL_NVP(wakeUpArea_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version <= 1)
                archive(cereal::base_class<ComponentBase>(this));
            else if (version >= 2)
                archive(cereal::base_class<NetworkComponent>(this));
            if (version >= 1) archive(CEREAL_NVP(chattingUi_));
            if (version >= 3) archive(CEREAL_NVP(featStep_));
            if (version >= 4) archive(CEREAL_NVP(interactableArea_));
            if (version >= 4) archive(CEREAL_NVP(wakeUpArea_));
        }
#pragma endregion
    };
    
    template <RequireType::Traits TraitsT>
    PlayerAvatarBase<TraitsT>::~PlayerAvatarBase()
    {
        
    }
    
    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::Init(
        std::shared_ptr<Status      > status,
        std::unique_ptr<StateMachine> stateMachine,
        std::shared_ptr<InputAction > inputAction,
        const std::weak_ptr<CameraGroup>& cameraGroup,
        const bool isOwner)
    {
        isOwner_       = isOwner;
        modelRenderer_ = RequireComponent<Component::ModelRenderer>();
        PlayerAvatars_().push_back(Components().Catch<IPlayerAvatar>());
        
        animatorComponent_ = RequireComponent<Component::Animator>();
        rigidBody_         = Components().Catch<Component::RigidBody>();
        
        status_       = std::move(status      );
        RegisterSyncObject(status_);
        inputAction_  = std::move(inputAction );
        stateMachine_ = std::move(stateMachine);
        animator_     = std::make_unique<Animator>(animatorComponent_);
        cameraGroup_  = cameraGroup;
        if (!cameraGroup_.expired())
            cameraGroup_.lock()->Init(Entity().lock());
        SubscribeStateToAnimator();
        status_->Init();
    }
    
    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::OnAwake()
    {
        
    }
    
    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::OnUpdate()
    {
        inputAction_ ->OnUpdate();
        stateMachine_->OnUpdate();
        status_      ->OnUpdate();
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::NetworkedTick()
    {
        stateMachine_->NetworkTick(GetNetworkObjectId(), HasStateAuthority());
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::OnFixedUpdate()
    {
        stateMachine_->OnFixedUpdate();
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::OnDestroy()
    {
        const auto ownPtr = Components().Catch<IPlayerAvatar>().lock();
            auto& playerAvatars = PlayerAvatars_();
            playerAvatars.erase(
                std::remove_if(playerAvatars.begin(), playerAvatars.end(),
                    [&](const std::weak_ptr<IPlayerAvatar>& weak)
                    {
                        return !weak.expired() && weak.lock() == ownPtr;
                    }),
                playerAvatars.end()
            );
    }
    
    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::SubscribeStateToAnimator()
    {
        stateMachine_->CurrentState()
            .Subscribe([this](const std::shared_ptr<State>& state)
            {
                animatedState_ = state;
                animator_->ChangeAnimation(state->AnimationType());
            }).AddTo(this);

        // NOTE: 負傷で見た目だけ変わるStateがあるので、State が変わらなくても貼り直す
        const auto reapply = [this](NanamiEngine::R4::Unit)
        {
            if (animatedState_)
                animator_->ChangeAnimation(animatedState_->AnimationType());
        };
        status_->OnBecomeInjured     ().Subscribe(reapply).AddTo(this);
        status_->OnRecoverFromInjured().Subscribe(reapply).AddTo(this);
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::ApplySyncState(uint8_t stateValue)
    {
        stateMachine_->ApplySyncState(stateValue);
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::SaveStatus()
    {
        // 力尽きたまま保存すると、次に生成した瞬間から倒れている。ゲームオーバー後は出発前の保存から始め直す
        if (status_->IsDeath())
            return;

        // 報酬を入れた所持金と、受注・達成の記録は同じ時点で保存する。
        // 間で落ちたときは報酬が消えるほうに倒す(受注が残っていれば読み直したときにもう一度達成できる)
        GameCore::PlayerAvatar::Quest::QuestJournal::Instance().Save();
        GameCore::PlayerAvatar::SaveStatus<Status, TraitsT>(status_);
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::BindQuestJournal()
    {
        auto& journal = GameCore::PlayerAvatar::Quest::QuestJournal::Instance();
        journal.Reload();
        journal.Adopt(status_->Quest().ReleaseLegacyQuests());
        journal.OnRewarded()
            .Subscribe([this](const GameCore::StatusParameter::Money& reward)
            {
                status_->Wallet().Earn(reward);
            }).AddTo(this);
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::EnableStateMachiine()
    {
        stateMachine_->OnEnable();
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::DisableStateMachine()
    {
        stateMachine_->OnDisable();
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::BasedOnDrawgui()
    {
        if (animator_)
            animator_->OnDrawGui();
        if (stateMachine_)
            stateMachine_->OnDrawGui();
        if (status_ && ImGui::TreeNode("Status"))
        {
            status_->OnDrawGui();
            ImGui::TreePop();
            ImGui::Spacing();
        }
    }
    
    template <RequireType::Traits TraitsT>
    InteractableArea& PlayerAvatarBase<TraitsT>::InteractableArea() const
    {
        if (!interactableArea_)
            throw std::exception("not found InteractableArea");

        return *interactableArea_.get();
    }

    template <RequireType::Traits TraitsT>
    WakeUpArea& PlayerAvatarBase<TraitsT>::WakeUpArea() const
    {
        if (!wakeUpArea_)
            throw std::exception("not found WakeUpArea");

        return *wakeUpArea_.get();
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::RequestWakeUp()
    {
        GameCore::Network::WakeUpPlayerRpc::Send(GetNetworkObjectId(), Core::Network::DeliveryMode::Reliable);
    }

    template <RequireType::Traits TraitsT>
    glm::vec3 PlayerAvatarBase<TraitsT>::CameraTargetPosition() const
    {
        if (const auto modelRenderer = modelRenderer_.lock())
            return modelRenderer->RenderWorldPos();
        return Transform().GetWorldPos();
    }

    template <RequireType::Traits TraitsT>
    const glm::vec3& PlayerAvatarBase<TraitsT>::FeatStepPosition() const
    {
        if (!featStep_)
            throw std::exception("not found featStepPosition");

        return featStep_->Transform().GetWorldPos();
    }

    // PlayerAvatarBase<Traits>をcerealに登録するマクロ
    // NOTE: PLAYER_AVATAR_BASE_CLASS_VERSION はヘッダ、REGISTER_PLAYER_AVATAR_BASE は .cpp に書く
#define PLAYER_AVATAR_BASE_CLASS_VERSION(TraitsType)                             \
CEREAL_CLASS_VERSION(                                                            \
GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::TraitsType>, 4)

#define REGISTER_PLAYER_AVATAR_BASE(TraitsType)                                  \
NANAMI_REGISTER_TYPE(                                                            \
GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::TraitsType>,    \
NanamiEngine::Module::Component::ComponentBase)
}