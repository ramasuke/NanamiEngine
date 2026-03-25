#pragma once
#include <typeindex>

#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../State/IPlayerAvatarState.h"
#include "../../State/Action/PlayerAvatarStateAction.h"
#include "../../State/Condition/PlayerAvatarStateCondition.h"
#include "../Animation/SwordManAvatarAnimation.h"
#include "../InputAction/SwordManAvatarInputAction.h"
#include "../Status/SwordManAvatarStatus.h"
#include "Context/SwordManAvatarStateContext.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStateBase : public IPlayerAvatarState
    {
    public:
        explicit SwordManAvatarStateBase(  const std::shared_ptr<SwordManAvatarStateContext>& context
                                         , const std::function<void(std::type_index)>& onChangeState);

        virtual ~SwordManAvatarStateBase() override = default;
        [[nodiscard]] virtual AnimationType AnimationType() const = 0;
        void OnEnter () override;
        void OnUpdate() override;
        void OnExit  () override;
        
    private:
        float stateDuring_secs_;
        std::shared_ptr<SwordManAvatarStateContext> context_;
        std::function<void(std::type_index)> onChangeState_;
        static inline const auto CHATTABLE_ICON_OBJECT_NAME = "ChattableIcon";

    protected:
        /** ---- 以下templateMethodパターン ---- */
        virtual void DoEnter () = 0;
        virtual void DoUpdate() = 0;
        virtual void DoExit  () = 0;
        
    protected:
        /** ---- 以下サンドボックスパターン ---- */
        /** @note Playerの行動に必要なパラメータと行動を取得できる関数群 */
        [[nodiscard]] GameObject::IGameObject   &            Player          () const { return *context_->PlayerAvatarObject    (); }
        [[nodiscard]] Physics::ICollider        &            Collider        () const { return context_->PlayerAvatarCollider   (); }
        [[nodiscard]] GameObject::Transform     &            Transform       () const { return context_->PlayerAvatarTransform  (); }
        [[nodiscard]] SwordManAvatarInputAction &            Input           () const { return context_->Input                  (); }
        [[nodiscard]] SwordManAvatarStatus      &            Status          () const { return context_->Status                 (); }
        [[nodiscard]] State::IStatusEventSubject&            StatusEvent     () const { return context_->Status().Subject       (); }
        [[nodiscard]] SwordManAvatarCameraGroup &            CameraGroup     () const { return context_->Camera                 (); }
        [[nodiscard]] GamePlay::Ui::NpcChatting &            NpcChattingUi   () const { return context_->NpcChattingUi          (); }
        [[nodiscard]] glm::vec3                              FeatStepPos     () const { return context_->PlayerAvatarFeatStepPos();}
        [[nodiscard]] GamePlay::PlayerAvatar::ChattableArea& ChattableArea   () const { return context_->ChattableArea          (); }
        [[nodiscard]] PlayerAttackArea& NormalAttackArea() const { return context_->NormalAttackArea(); }
        [[nodiscard]] PlayerAttackArea& DashAttackArea  () const { return context_->DashAttackArea  (); }
        [[nodiscard]] Component::ParticleSystem& OnReinforceParticle() const { return context_->OnReinforceParticle(); }
        [[nodiscard]] Component::ParticleSystem& ReinforcingParticle() const { return context_->ReinforcingParticle(); }
        [[nodiscard]] PlayerAvatar::State::PlayerAvatarStateCondition Conditions() const { return PlayerAvatar::State::PlayerAvatarStateCondition(context_);}
        [[nodiscard]] PlayerAvatar::State::PlayerAvatarStateAction    Actions   () const { return PlayerAvatar::State::PlayerAvatarStateAction   (context_);}
        [[nodiscard]] const Asset::SoundFile&                NormalAttackSound() const { return context_->NormalAttackSound(); } 
        [[nodiscard]] const Asset::SoundFile&                AvoidRollingSound() const { return context_->AvoidRollingSound(); } 
        
        //現在のStateの持続時間を返す
        [[nodiscard]] float During_secs() const { return stateDuring_secs_; }
        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const;
        template<typename StateT> requires std::derived_from<StateT, SwordManAvatarStateBase> void OnTryChangeState(const std::function<bool()>& check) const;
        template<typename StateT> requires std::derived_from<StateT, SwordManAvatarStateBase> void OnTryChangeState(bool check) const;
        template<typename StateT> requires std::derived_from<StateT, SwordManAvatarStateBase> void OnChangeState() const;
        template<typename T>
        [[nodiscard]] T& CatchPlayerInChild(const std::string& catchObjectName) const
        {
            for (const auto& child : Transform().GetAllChildren())
            {
                if (child->Name() != catchObjectName)
                    continue;

                const auto object = child->Components().Catch<T>().lock();
                assert(object, "Object has not T");
                
                return *object;
            }
            throw std::exception(("Object has not (object name:" + catchObjectName + ")").c_str());
        }

protected:
//Stateコンストラクタの生成マクロ
#define DEFINE_STATE_CONSTRUCTOR(DerivedClass) \
explicit DerivedClass( \
    const std::shared_ptr<GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateContext>& context,\
    std::function<void(std::type_index)> onChangeState) \
    : SwordManAvatarStateBase(context, onChangeState) {}
};

    template <typename StateT>
    requires std::derived_from<StateT, SwordManAvatarStateBase>
    void SwordManAvatarStateBase::OnTryChangeState(const std::function<bool()>& check) const
    {
        if (check())
        {
            OnChangeState<StateT>();
        }   
    }

    template <typename StateT>
    requires std::derived_from<StateT, SwordManAvatarStateBase>
    void SwordManAvatarStateBase::OnTryChangeState(const bool check) const
    {
        if (check)
        {
            OnChangeState<StateT>();
        }
    }

    template <typename StateT>
        requires std::derived_from<StateT, SwordManAvatarStateBase>
    void SwordManAvatarStateBase::OnChangeState() const
    {
        onChangeState_(typeid(StateT));
    }
}
