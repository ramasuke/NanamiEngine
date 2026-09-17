#pragma once
#include <string>
#include <vector>

#include "SwordManAvatarStateType.h"
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../State/IPlayerAvatarState.h"
#include "../../State/Action/PlayerAvatarStateAction.h"
#include "../../State/Condition/PlayerAvatarStateCondition.h"
#include "../Animation/SwordManAvatarAnimation.h"
#include "../InputAction/SwordManAvatarInputAction.h"
#include "../Status/SwordManAvatarStatus.h"
#include "Context/SwordManAvatarStateContext.h"
#include "Transition/SwordManAvatarStateTransition.h"

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
        [[nodiscard]] virtual SwordManAvatarControlAcceptance ControlAcceptance() const = 0;
        /** @brief このStateから起こりうる遷移とState内の操作を評価順に宣言する。副作用を持たせないこと */
        virtual void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const {}
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
        std::vector<bool> footstepBoneAirborne_; ///< 足ボーンごとの「接地高さより上に離れた」ラッチ（enter でクリア）
        float currentMoveSpeed_ = 0.0f;
        float decelerationStartSpeed_ = 0.0f;
        std::weak_ptr<GameObject::IGameObject> attackAutoAimTarget_; ///< 非ロックオン時に攻撃で向く相手。State中はこの1体に固定する
        float attackYawVelocity_ = 0.0f;
        void UseSelectedPouchItem() const;
        [[nodiscard]] bool IsLockOnTargetInRange() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> FindNearestLockOnTarget() const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> ResolveAttackTarget();
        [[nodiscard]] bool HasLineOfSight(const std::shared_ptr<GameObject::IGameObject>& target) const;

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
        [[nodiscard]] Component::RigidBody      &            RigidBody       () const { return context_->PlayerAvatarRigidBody  (); }
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
        [[nodiscard]] Component::ParticleSystem& SuccessAvoidRollingParticle() const { return context_->SuccessAvoidRollingParticle(); }

        void ResetDuringTime();
        //現在のStateの持続時間を返す
        [[nodiscard]] float During_secs() const { return stateDuring_secs_; }
        /**
         * @brief 足ボーンが接地高さまで降りてきたフレームで、その足の位置にパーティクルと足音を出す
         * @param footstepSounds 足音の候補配列。鳴らすときにこの中からランダムで1つ選ばれる。空なら足音なし
         * @note 見ているボーンと接地高さは SwordManAvatarResource 側の設定。クリップの尺に依存せず実際の足の高さで鳴る
         */
        void TryEmitFootstep(const std::vector<FIELD(Asset::SoundFile)>& footstepSounds);
        /** @brief 候補からランダムに1つ選んでpositionで鳴らす。候補が空なら何もしない */
        void PlayRandomSe(const std::vector<FIELD(Asset::SoundFile)>& sounds, const glm::vec3& position) const;
        /**
         * @brief 攻撃の当たり外れで音を鳴らし分ける。敵に当たったら打撃音、外したら風切り音
         * @note 当たり判定を取った直後に呼ぶ想定。段ごとの音を持つ攻撃は、段の音がないときの受け皿にする
         */
        void PlayAttackSe(bool isHit) const;
        /** @brief 移動速度の初期値を物理ボディの現在の水平速度にする。移動系ステートの DoEnter で呼ぶ */
        void ResetMoveSpeedFromVelocity();
        /**
         * @brief 水平速度を0にして、その場に留まらせる
         * @note DoEnter で1回潰すだけだと重力で斜面を滑り出すので、留まるステートは毎 DoFixedUpdate で呼ぶ
         */
        void HoldHorizontalVelocity() const;
        /**
         * @brief 自機の向き(-Z)へ speed で水平移動する。落下速度(Y)はそのまま残す
         * @note 斜面や衝突で速度が削られるので、踏み込み中は毎 DoFixedUpdate で呼ぶ
         */
        void LungeForward(float speed) const;
        /**
         * @brief 移動速度を maxSpeed へ線形に加速/減速させながら入力方向へ移動する
         * @param accelerationTime_secs 0から maxSpeed に達するまでの時間。0以下なら即 maxSpeed
         * @param decelerationTime_secs ResetMoveSpeedFromVelocity 時点の速度から maxSpeed まで落としきる時間。0以下なら即 maxSpeed
         */
        void MoveForward(StatusParameter::MoveSpeed maxSpeed, float accelerationTime_secs, float decelerationTime_secs);
        void ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const;
        /**
         * @brief LockOn入力の読み取り・トグル・自動解除をまとめて処理する
         * @note ロック中に対象が死亡/索敵範囲外になった場合は自動でFollowFromBehindへ戻す
         */
        void UpdateLockOn() const;
        /**
         * @brief アイテムの切替と使用の入力を処理する
         * @note VisitTransitions で CycleItem / UseItem を宣言している State だけが呼ぶこと。
         *       アイテム欄はその宣言を見て出入りするので、宣言と呼び出しがずれると表示と操作が食い違う
         */
        void UpdateItemPouchInput() const;
        /** @brief 砥石などの攻撃力バフを掛けた威力。当たり判定とダメージ表示の両方をこれに通す */
        [[nodiscard]] Damage::PhysicsPower BuffedAttackPower(Damage::PhysicsPower base) const;
        void VisitLockOnAction(ISwordManAvatarTransitionVisitor& visitor) const;
        /**
         * @brief VisitTransitions の宣言どおりに遷移する
         * @return 1つでも遷移したか
         */
        bool UpdateTransitions() const;
        /**
         * @brief 攻撃対象(ロックオン中はロックオン対象、それ以外は最寄りのロックオン候補)へ臨界減衰バネで向く
         * @param smoothTime_secs 目標角へ追いつくまでのおおよその時間
         * @param maxRotateSpeed  最大角速度 [rad/s]
         * @note 対象なし / カメラ失効時は何もしない。攻撃の予備動作中に呼ぶ想定
         */
        void RotateTowardsAttackTarget(float smoothTime_secs, float maxRotateSpeed);
        /** @brief 攻撃で向く対象と回転の角速度をリセットする。次の RotateTowardsAttackTarget で対象を選び直す */
        void ResetAttackRotation();
        /** @brief attackAreaが捉えている対象それぞれへダメージ数値テキストを表示する */
        void DealDamageText(PlayerAttackArea& attackArea, Damage::PhysicsPower power) const;
        /**
         * @brief attackAreaが捉えている対象のうちPlayerHitShakeReceiverを持つものを、自機から対象への水平方向に揺らす
         * @note 描画位置だけを揺らすローカル演出(同期しない・Transformや物理は動かさない)
         */
        void ShakeHitTargets(PlayerAttackArea& attackArea, const HitFeelParam& hitFeel) const;
        /**
         * @brief 空振りした攻撃が壁に阻まれていたら、衝突点に火花を出してAttackedShockedへ遷移する
         * @return 壁に弾かれたか
         * @note 攻撃が何にもヒットしなかったときに呼ぶ想定
         */
        bool TryBlockAttackByWall(PlayerAttackArea& attackArea) const;
        void OnChangeState   (SwordManAvatarStateType type) const;
        void OnTryChangeState(SwordManAvatarStateType type, const std::function<bool()>& check) const;
        void OnTryChangeState(SwordManAvatarStateType type, bool check) const;

    protected:
        //State Ctor Generated macro
            #define DEFINE_STATE_CONSTRUCTOR(DerivedClass) \
            explicit DerivedClass( \
            const std::shared_ptr<GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateContext>& context, \
            const std::function<void(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateType)>& onChangeState) \
            : SwordManAvatarStateBase(context, onChangeState) {}
    };
}
