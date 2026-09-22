#pragma once
#include <functional>
#include <memory>

#include "vec3.hpp"
#include "IPlayerAvatarState.h"
#include "Action/PlayerAvatarStateAction.h"
#include "Condition/PlayerAvatarStateCondition.h"
#include "Transition/PlayerAvatarStateTransition.h"
#include "../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "../Input/PlayerAvatarInput_void.h"
#include "../LockOnTarget/PlayerAvatarLockOn.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"

namespace NanamiEngine::Module::Component
{
    class Animator;
}

namespace GameCore::PlayerAvatar
{
    //NOTE: 値オブジェクト
    template <typename ContextT, typename StateTypeT>
    struct PlayerAvatarStateArgs final
    {
        explicit PlayerAvatarStateArgs(const std::shared_ptr<ContextT>& context, const std::function<void(StateTypeT)>& onChangeState)
            : context_      (context      )
            , onChangeState_(onChangeState)
        {
        }

        [[nodiscard]] const std::shared_ptr<ContextT>&       Context      () const { return context_;       }
        [[nodiscard]] const std::function<void(StateTypeT)>& OnChangeState() const { return onChangeState_; }

    private:
        std::shared_ptr<ContextT>       context_;
        std::function<void(StateTypeT)> onChangeState_;
    };

    [[nodiscard]] Component::Animator& CatchPlayerAvatarAnimator(GameObject::IGameObject& playerAvatar);

    /**
     * @brief PlayerAvatar の State に共通する処理
     */
    template <typename ContextT, typename StateTypeT, typename AnimationTypeT, typename TransitionVisitorT>
    class PlayerAvatarStateBase : public IPlayerAvatarState
    {
    public:
        using Args = PlayerAvatarStateArgs<ContextT, StateTypeT>;

        explicit PlayerAvatarStateBase(const Args& args)
            : stateDuring_secs_(0.0f                )
            , context_         (args.Context()      )
            , onChangeState_   (args.OnChangeState())
        {
        }

        virtual ~PlayerAvatarStateBase() override = default;
        [[nodiscard]] virtual AnimationTypeT AnimationType() const = 0;
        [[nodiscard]] virtual PlayerAvatarControlAcceptance ControlAcceptance() const = 0;
        /** @brief このStateから起こりうる遷移とState内の操作を評価順に宣言する。副作用を持たせないこと */
        virtual void VisitTransitions(TransitionVisitorT& visitor) const {}

        void OnEnter() override
        {
            ResetDuringTime();
            DoEnter();
        }

        void OnUpdate() override
        {
            DoUpdate();
            stateDuring_secs_ += Time::DeltaTime();
        }

        void OnFixedUpdate() override
        {
            DoFixedUpdate();
        }

        void OnExit() override
        {
            DoExit();
        }

    private:
        float stateDuring_secs_;
        std::shared_ptr<ContextT> context_;
        std::function<void(StateTypeT)> onChangeState_;

    protected:
        /** ---- 以下templateMethodパターン ---- */
        virtual void DoEnter      () = 0;
        virtual void DoUpdate     () = 0;
        virtual void DoFixedUpdate() = 0;
        virtual void DoExit       () = 0;

        /** @brief UpdateLockOn で新しくロックオンした時に呼ばれる */
        virtual void OnLockOnEngaged() const {}

    protected:
        /** ---- 以下サンドボックスパターン ---- */
        /** @note Playerの行動に必要なパラメータと行動を取得できる関数群 */
        [[nodiscard]] ContextT&                             Context      () const { return *context_; }
        [[nodiscard]] GameObject::IGameObject&              Player       () const { return *Context().PlayerAvatarObject(); }
        [[nodiscard]] Component::Animator&                  Animator     () const { return CatchPlayerAvatarAnimator(Player()); }
        [[nodiscard]] Component::RigidBody&                 RigidBody    () const { return Context().PlayerAvatarRigidBody(); }
        [[nodiscard]] GameObject::Transform&                Transform    () const { return Context().PlayerAvatarTransform(); }
        [[nodiscard]] auto&                                 Input        () const { return Context().Input(); }
        [[nodiscard]] auto&                                 Status       () const { return Context().Status(); }
        [[nodiscard]] auto&                                 CameraGroup  () const { return Context().Camera(); }
        [[nodiscard]] bool                                  ExpiredCamera() const { return Context().ExpiredCamera(); }
        [[nodiscard]] const auto&                           Resources    () const { return Context().Resources(); }
        [[nodiscard]] GamePlay::PlayerAvatar::InteractableArea& InteractableArea() const { return Context().InteractableArea(); }
        [[nodiscard]] State::PlayerAvatarStateCondition     Conditions   () const { return State::PlayerAvatarStateCondition(context_); }
        [[nodiscard]] State::PlayerAvatarStateAction        Actions      () const { return State::PlayerAvatarStateAction   (context_); }

        void ResetDuringTime() { stateDuring_secs_ = 0.0f; }
        //現在のStateの持続時間を返す
        [[nodiscard]] float During_secs() const { return stateDuring_secs_; }
        void OnChangeState(const StateTypeT type) const { onChangeState_(type); }
        [[nodiscard]] const std::function<void(StateTypeT)>& OnChangeStateCallback() const { return onChangeState_; }

        // 1回潰すだけだと重力で斜面を滑り出すので、留まるStateは毎 DoFixedUpdate で呼ぶ
        void HoldHorizontalVelocity() const
        {
            RigidBody().SetLinearVelocity(glm::vec3(0.0f, RigidBody().LinearVelocity().y, 0.0f));
        }

        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const
        {
            Context().Camera().ChangeCamera(camera);
        }

        /** @brief ロックオン中はロックオンカメラを維持し、そうでなければ背後のカメラへ切り替える */
        void ChangeCameraByLockOn() const
        {
            if (ExpiredCamera())
                return;

            auto& cameraGroup = Context().Camera();
            ChangeCamera(cameraGroup.IsLockedOn() ? cameraGroup.LockOnCamera() : cameraGroup.FollowFromBehind());
        }

        /** @brief LockOn 入力のトグル、ロック中の左右切り替え、対象が見えなくなった時の自動解除 */
        void UpdateLockOn() const
        {
            if (ExpiredCamera() || Context().ExpiredLockOnDetectionArea())
                return;

            auto& input = Context().Input();
            const int switchDirection = static_cast<int>(input.LockOnSwitchRight().IsPressed()) - static_cast<int>(input.LockOnSwitchLeft().IsPressed());
            if (LockOn::Update(Context().Camera(), Context().LockOnDetectionArea(), Transform().GetWorldPos(), input.LockOn().IsPressed(), switchDirection))
                OnLockOnEngaged();
        }

        void VisitLockOnAction(TransitionVisitorT& visitor) const
        {
            if (ExpiredCamera())
                return;

            using ActionType = typename TransitionVisitorT::ActionType;
            const auto& cameraGroup = Context().Camera();
            const bool isLockedOn = cameraGroup.IsLockedOn();
            visitor.Action(
                isLockedOn ? ActionType::LockOnRelease : ActionType::LockOn,
                isLockedOn || !cameraGroup.LockOnCandidate().expired());
        }

        /** @brief 真上へ跳び、ジャンプのクールダウンとスタミナ消費を始める */
        void ApplyJump() const
        {
            auto& status = Context().Status();
            Actions().Jump(glm::vec3{0, 1, 0} * status.GetJumpPower());
            status.StartJumpCooldown();
            status.ConsumeJumpStamina();
        }
    };
}
