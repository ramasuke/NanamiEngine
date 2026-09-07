#pragma once
#include <string>
#include <vector>

#include "SwordManAvatarStateType.h"
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../State/IPlayerAvatarState.h"
#include "../../State/Action/PlayerAvatarStateAction.h"
#include "../../State/Condition/PlayerAvatarStateCondition.h"
#include "../Animation/SwordManAvatarAnimation.h"
#include "../InputAction/SwordManAvatarInputAction.h"
#include "../Status/SwordManAvatarStatus.h"
#include "Context/SwordManAvatarStateContext.h"

namespace NanamiEngine::Module::Component
{
    class Animator;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStateBase : public IPlayerAvatarState
    {
    public:
        explicit SwordManAvatarStateBase(  const std::shared_ptr<SwordManAvatarStateContext>& context
                                         , const std::function<void(SwordManAvatarStateType)>& onChangeState);

        virtual ~SwordManAvatarStateBase() override = default;
        [[nodiscard]] virtual AnimationType AnimationType() const = 0;
        void OnEnter      () override;
        void OnUpdate     () override;
        void OnFixedUpdate() override;
        void OnExit       () override;
        [[nodiscard]] virtual bool MouseLock() { return true; }

    private:
        float stateDuring_secs_;
        std::shared_ptr<SwordManAvatarStateContext> context_;
        std::function<void(SwordManAvatarStateType)> onChangeState_;
        static inline const auto CHATTABLE_ICON_OBJECT_NAME = "ChattableIcon";
        float prevFootstepNormalizedTime_ = -1.0f; ///< 前フレームのクリップ正規化時間（enter で -1 リセット）
        [[nodiscard]] bool IsLockOnTargetInRange() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindNearestLockOnTarget() const;

    protected:
        /** ---- 以下templateMethodパターン ---- */
        virtual void DoEnter      () = 0;
        virtual void DoUpdate     () = 0;
        virtual void DoFixedUpdate() = 0;
        virtual void DoExit       () = 0;

    protected:
        /** ---- 以下サンドボックスパターン ---- */
        /** @note Playerの行動に必要なパラメータと行動を取得できる関数群 */
        [[nodiscard]] GameObject::IGameObject   &            Player          () const { return *context_->PlayerAvatarObject    (); }
        [[nodiscard]] Component::Animator       &            Animator        () const;
        [[nodiscard]] Component::ColliderBase   &            Collider        () const { return context_->PlayerAvatarCollider   (); }
        [[nodiscard]] GameObject::Transform     &            Transform       () const { return context_->PlayerAvatarTransform  (); }
        [[nodiscard]] SwordManAvatarInputAction &            Input           () const { return context_->Input                  (); }
        [[nodiscard]] SwordManAvatarStatus      &            Status          () const { return context_->Status                 (); }
        [[nodiscard]] State::IStatusEventSubject&            StatusEvent     () const { return context_->Status().Subject       (); }
        [[nodiscard]] SwordManAvatarCameraGroup &            CameraGroup     () const { return context_->Camera                 (); }
        [[nodiscard]] bool                                   ExpiredCamera   () const { return context_->ExpiredCamera(); }
        [[nodiscard]] GamePlay::Ui::NpcChatting &            NpcChattingUi   () const { return context_->NpcChattingUi          (); }
        [[nodiscard]] glm::vec3                              FeatStepPos     () const { return context_->PlayerAvatarFeatStepPos();}
        [[nodiscard]] GamePlay::PlayerAvatar::ChattableArea& ChattableArea   () const { return context_->ChattableArea          (); }
        [[nodiscard]] GamePlay::PlayerAvatar::WakeUpArea   & WakeUpArea      () const { return context_->WakeUpArea             (); }
        [[nodiscard]] PlayerAttackArea& NormalAttackArea   () const { return context_->NormalAttackArea   (); }
        [[nodiscard]] PlayerAttackArea& DashAttackArea     () const { return context_->DashAttackArea     (); }
        [[nodiscard]] GamePlay::PlayerAvatar::LockOnDetectionArea& LockOnDetectionArea() const { return context_->LockOnDetectionArea(); }
        [[nodiscard]] PlayerAvatar::State::PlayerAvatarStateCondition Conditions() const { return PlayerAvatar::State::PlayerAvatarStateCondition(context_);}
        [[nodiscard]] PlayerAvatar::State::PlayerAvatarStateAction    Actions   () const { return PlayerAvatar::State::PlayerAvatarStateAction   (context_);}
        [[nodiscard]] const Asset::SwordManAvatarResource&            Resources () const { return context_->Resources(); }

        void ResetDuringTime();
        //現在のStateの持続時間を返す
        [[nodiscard]] float During_secs() const { return stateDuring_secs_; }
        /**
         * @brief 現在再生中クリップの正規化再生時間が接地フェーズを跨いだフレームで、足元にパーティクルと足音を出す
         * @param contactPhases クリップ正規化時間 [0,1) の接地タイミング配列。空なら既定値を使用
         * @param footstepSounds 足音の候補配列。鳴らすときにこの中からランダムで1つ選ばれる。空なら足音なし
         */
        void TryEmitFootstep(const std::vector<float>& contactPhases,
                             const std::vector<FIELD(Asset::SoundFile)>& footstepSounds);
        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const;
        /**
         * @brief LockOn入力の読み取り・トグル・自動解除をまとめて処理する
         * @note ロック中に対象が死亡/索敵範囲外になった場合は自動でFollowFromBehindへ戻す
         */
        void UpdateLockOn() const;
        void OnChangeState   (SwordManAvatarStateType type) const;
        void OnTryChangeState(SwordManAvatarStateType type, const std::function<bool()>& check) const;
        void OnTryChangeState(SwordManAvatarStateType type, bool check) const;
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
        //State Ctor Generated macro
            #define DEFINE_STATE_CONSTRUCTOR(DerivedClass) \
            explicit DerivedClass( \
            const std::shared_ptr<GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateContext>& context, \
            const std::function<void(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateType)>& onChangeState) \
            : SwordManAvatarStateBase(context, onChangeState) {}
    };
}
