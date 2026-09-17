#pragma once
#include <functional>
#include <memory>

#include "MagicCasterAvatarStateType.h"
#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../State/IPlayerAvatarState.h"
#include "../../State/Action/PlayerAvatarStateAction.h"
#include "../../State/Condition/PlayerAvatarStateCondition.h"
#include "../Animation/MagicCasterAvatarAnimation.h"
#include "../InputAction/MagicCasterAvatarInputAction.h"
#include "../Status/MagicCasterAvatarStatus.h"
#include "Context/MagicCasterAvatarStateContext.h"

namespace NanamiEngine::Module::Component
{
    class Animator;
}

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarStateBase : public IPlayerAvatarState
    {
    public:
        explicit MagicCasterAvatarStateBase(  const std::shared_ptr<MagicCasterAvatarStateContext>& context
                                             , const std::function<void(MagicCasterAvatarStateType)>& onChangeState);

        virtual ~MagicCasterAvatarStateBase() override = default;
        [[nodiscard]] virtual AnimationType AnimationType() const = 0;
        void OnEnter      () override;
        void OnUpdate     () override;
        void OnFixedUpdate() override;
        void OnExit       () override;

    private:
        float stateDuring_secs_;
        std::shared_ptr<MagicCasterAvatarStateContext> context_;
        std::function<void(MagicCasterAvatarStateType)> onChangeState_;

    protected:
        /** ---- 以下templateMethodパターン ---- */
        virtual void DoEnter      () = 0;
        virtual void DoUpdate     () = 0;
        virtual void DoFixedUpdate() = 0;
        virtual void DoExit       () = 0;

    protected:
        /** ---- 以下サンドボックスパターン ---- */
        [[nodiscard]] GameObject::IGameObject     &          Player      () const { return *context_->PlayerAvatarObject(); }
        [[nodiscard]] Component::Animator         &          Animator    () const;
        [[nodiscard]] Component::RigidBody        &          RigidBody   () const { return context_->PlayerAvatarRigidBody(); }
        [[nodiscard]] GameObject::Transform       &          Transform   () const { return context_->PlayerAvatarTransform(); }
        [[nodiscard]] MagicCasterAvatarInputAction&          Input       () const { return context_->Input();  }
        [[nodiscard]] MagicCasterAvatarStatus     &          Status      () const { return context_->Status(); }
        [[nodiscard]] PlayerAvatarCameraGroupBase &          CameraGroup () const { return context_->Camera(); }
        [[nodiscard]] bool                                   ExpiredCamera() const { return context_->ExpiredCamera(); }
        [[nodiscard]] const Asset::MagicCasterAvatarResource& Resources  () const { return context_->Resources(); }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject>  CastPoint  () const { return context_->CastPoint(); }
        [[nodiscard]] PlayerAvatar::State::PlayerAvatarStateCondition Conditions() const { return PlayerAvatar::State::PlayerAvatarStateCondition(context_); }
        [[nodiscard]] PlayerAvatar::State::PlayerAvatarStateAction    Actions   () const { return PlayerAvatar::State::PlayerAvatarStateAction   (context_); }

        void ResetDuringTime();
        //現在のStateの持続時間を返す
        [[nodiscard]] float During_secs() const { return stateDuring_secs_; }
        /** @brief 水平速度を0にして、その場に留まらせる */
        void HoldHorizontalVelocity() const;
        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const;
        void OnChangeState(MagicCasterAvatarStateType type) const;

    protected:
        //State Ctor Generated macro
            #define DEFINE_MAGICCASTER_STATE_CONSTRUCTOR(DerivedClass) \
            explicit DerivedClass( \
            const std::shared_ptr<GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStateContext>& context, \
            const std::function<void(GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStateType)>& onChangeState) \
            : MagicCasterAvatarStateBase(context, onChangeState) {}
    };
}
