#pragma once
#include <utility>

#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "../../Core/Game/Npc/Enemy/ITakableEnemyAttack/ITakableEnemyAttack.h"
#include "../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../Core/Game/PlayerAvatar/StateMachine/PlayerAvatarStateMachine.h"
#include "../../Core/Game/PlayerAvatar/RequireType/RequireType.h"
#include "../../Core/Game/PlayerAvatar/Status/PlayerAvatarStatus.h"
#include "../Ui/NpcChatting/NpcChatting.h"
#include "ChattableArea/ChattableArea.h"
#include "../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::PlayerAvatar
{
    using namespace GameCore::PlayerAvatar;
    template <RequireType::Traits TraitsT>
    class PlayerAvatarBase : public Component::ComponentBase,
                             public LifeCycleCallback::IAwakable,
                             public LifeCycleCallback::IUpdatable,
                             public GameCore::IPlayerAvatar,
                             public GameCore::Npc::Enemy::ITakableEnemyAttack
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
                  const std::weak_ptr<CameraGroup>& cameraGroup);
        [[nodiscard]] IPlayerAvatarEventSceneStateMachine& GetEventSceneStateMachine() const override { return *stateMachine_; }
        /** @brief PlayerAvatar<T>のCameraを取得 */
        [[nodiscard]] Physics::ICollider& Collider() const override { return *collider_.lock(); }
        [[nodiscard]] const GameObject::Transform& PlayerTransform() const override { return Transform(); }
        [[nodiscard]] Status& PlayerStatus() const override { return *status_; }
        void SaveStatus() override;
        

    private:
        void OnAwake                 () override;
        void OnUpdate                () override;
        void OnDestroy               () override;
        void BasedOnDrawgui          () override;
        void SubscribeStateToAnimator();
        void OnTakeDamage(std::unique_ptr<GameCore::IDamage> context) override
        {
            status_->AddOnDamageStack(std::move(context));
        }

        [[nodiscard]] Ui::NpcChatting            & NpcChattingUi   () const override { return *chattingUi_.get(); }
        [[nodiscard]] PlayerAvatar::ChattableArea& ChattableArea   () const override;
        [[nodiscard]] const glm::vec3            & FeatStepPosition() const override;

        std::weak_ptr<Component::Animator> animatorComponent_;

        std::unique_ptr<Animator          > animator_     = nullptr;
        std::unique_ptr<StateMachine      > stateMachine_ = nullptr;
        std::shared_ptr<Status            > status_       = nullptr;
        std::shared_ptr<InputAction       > inputAction_  = nullptr;
        std::weak_ptr  <CameraGroup       > cameraGroup_;
        std::weak_ptr  <Physics::ICollider> collider_   ;
        [[serialize(0)]] FIELD(Ui::NpcChatting) chattingUi_;
        
    protected:
        /** 以下サンドボックスパターン */
        [[nodiscard]] std::shared_ptr<Physics::ICollider> CatchAttackArea(const std::string& childName) const;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(chattingUi_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(chattingUi_));
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
        const std::weak_ptr<CameraGroup>& cameraGroup)
    {
        RequireComponent<Component::ModelRenderer>();
        PlayerAvatars_().push_back(Components().Catch<IPlayerAvatar>());
        
        animatorComponent_ = RequireComponent<Component::Animator>();
        collider_          = Components().Catch<Physics::ICollider>();
        
        status_       = std::move(status      );
        inputAction_  = std::move(inputAction );
        stateMachine_ = std::move(stateMachine);
        animator_     = std::make_unique<Animator>(animatorComponent_);
        cameraGroup_  = cameraGroup;
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
        if (Transform().GetWorldPos().y < -100)
        {
            Transform().SetLocalPos(glm::vec3{0.0f, 100.0f, 0.0f});
        }
        stateMachine_->OnUpdate();
        inputAction_ ->OnUpdate();
        status_      ->OnUpdate();
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
            .subscribe([this](const std::shared_ptr<State>& state)
            {
                animator_->ChangeAnimation(state->AnimationType());
            },
            [](const std::exception_ptr&){ },
            []{ }
        );
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::SaveStatus()
    {
        GameCore::PlayerAvatar::SaveStatus<Status, TraitsT>(status_);
    }

    template <RequireType::Traits TraitsT>
    void PlayerAvatarBase<TraitsT>::BasedOnDrawgui()
    {
        if (animator_)
            animator_->OnDrawGui();
        if (stateMachine_)
            stateMachine_->OnDrawGui();
        if (status_)
            status_->OnDrawGui();
    }
    
    template <RequireType::Traits TraitsT>
    ChattableArea& PlayerAvatarBase<TraitsT>::ChattableArea() const
    {
        for (const auto& child : Transform().GetChildren())
        {
            if (const auto chattableArea = child->Components().Catch<PlayerAvatar::ChattableArea>().lock())
                return *chattableArea;
        }
        throw std::exception("not found ChattableArea");
    }
    
    template <RequireType::Traits TraitsT>
    const glm::vec3& PlayerAvatarBase<TraitsT>::FeatStepPosition() const
    {
        for (const auto& child : Transform().GetChildren())
        {
            if (child->Name() == "FeatStep")
                return child->Transform().GetWorldPos(); 
        }
        throw std::exception("not found featStepPosition");
    }
    
    template <RequireType::Traits TraitsT>
    std::shared_ptr<Physics::ICollider> PlayerAvatarBase<TraitsT>::CatchAttackArea(const std::string& childName) const
    {
        for (const auto& child : Transform().GetParent()->Transform().GetAllChildren())
        {
            if (child->Name() == childName)
            {
                return child->Components().Catch<Physics::ICollider>().lock();
            }
        }
        assert(false && "attackArea not found!");
        return nullptr;
    }

    // PlayerAvatarBase<Traits>をcerealに登録するマクロ
#define REGISTER_PLAYER_AVATAR_BASE(TraitsType)                                  \
CEREAL_CLASS_VERSION(                                                            \
GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::TraitsType>, 1) \
CEREAL_REGISTER_TYPE(                                                            \
GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::TraitsType>)    \
CEREAL_REGISTER_POLYMORPHIC_RELATION(                                            \
NanamiEngine::Module::Component::ComponentBase,                                  \
GamePlay::PlayerAvatar::PlayerAvatarBase<GameCore::PlayerAvatar::TraitsType>)
}