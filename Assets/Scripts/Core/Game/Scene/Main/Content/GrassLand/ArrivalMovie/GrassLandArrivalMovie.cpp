#include "GrassLandArrivalMovie.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "DxLib.h"

#include "../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../../../../../Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "../../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../../../../../../Libs/tweeny/Tweeny/tweeny.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../PlayerAvatar/StateMachine/EventScene/PlayerAvatarEventSceneStateType.h"
#include "../Context/GrassLandSceneContext.h"

namespace GameCore::Scene::GrassLand
{
    namespace
    {
        namespace ArrivalPhysics = NanamiEngine::Module::Physics;

        // 演出中だけプレイヤーのFollowFromBehind(0)より上に出す
        constexpr int ARRIVAL_CAMERA_PRIORITY = 100;
        constexpr unsigned char ARRIVAL_SKIP_TRIGGER_DEAD_ZONE = 30;
        // 0まで潰すとEffekseerの行列が壊れるので、閉じきりでもわずかに残す
        constexpr float ARRIVAL_PORTAL_MIN_OPEN_RATE = 0.001f;
        // 地面は基準の高さの少し上から真下へレイを飛ばして探す。スポーン地点のマーカーは地面から浮いている
        constexpr float ARRIVAL_GROUND_PROBE_HEIGHT   = 20.0f;
        constexpr float ARRIVAL_GROUND_PROBE_DISTANCE = 120.0f;

        /** @brief tweenyは尺0の区間で0/0になりNaNを返すので、最短でも1msにする */
        int ArrivalDuring_msecs(const int msecs) { return (std::max)(msecs, 1); }

        float ArrivalGroundY(const glm::vec3& pos, const float referenceY)
        {
            const glm::vec3 origin(pos.x, referenceY + ARRIVAL_GROUND_PROBE_HEIGHT, pos.z);
            const auto hit = ArrivalPhysics::Raycast(
                origin,
                glm::vec3(0.0f, -1.0f, 0.0f),
                ARRIVAL_GROUND_PROBE_DISTANCE,
                ArrivalPhysics::ToMask(ArrivalPhysics::Layer::Default));
            return hit.Hit() ? hit.Position().y : referenceY;
        }

        bool ArrivalIsSkipInputDown()
        {
            if (CheckHitKeyAll(DX_CHECKINPUT_KEY) != 0)
                return true;

            XINPUT_STATE xInput = {};
            if (GetJoypadXInputState(DX_INPUT_PAD1, &xInput) != 0)
                return false;

            if (xInput.LeftTrigger > ARRIVAL_SKIP_TRIGGER_DEAD_ZONE || xInput.RightTrigger > ARRIVAL_SKIP_TRIGGER_DEAD_ZONE)
                return true;

            for (const auto button : xInput.Buttons)
            {
                if (button != 0)
                    return true;
            }
            return false;
        }
    }

    GrassLandArrivalMovie::GrassLandArrivalMovie(
          const std::weak_ptr<IPlayerAvatar>& playerAvatar
        , const std::shared_ptr<GrassLandSceneContext>& context)
        : playerAvatar_(playerAvatar)
        , context_     (context     )
    {
    }

    void GrassLandArrivalMovie::Begin()
    {
        const auto context = context_.lock();
        const auto avatar  = playerAvatar_.lock();
        if (!context || !avatar)
            return;
        
        const glm::vec3 facing = avatar->PlayerTransform().GetWorldRot() * glm::vec3(0.0f, 0.0f, -1.0f);
        const glm::vec3 flatFacing(facing.x, 0.0f, facing.z);
        if (glm::dot(flatFacing, flatFacing) > 0.0001f)
            forward_ = glm::normalize(flatFacing);
        side_ = glm::vec3(-forward_.z, 0.0f, forward_.x);
        const glm::quat facingRot = glm::angleAxis(std::atan2(-forward_.x, -forward_.z), glm::vec3(0.0f, 1.0f, 0.0f));

        const glm::vec3 spawnPos = context->PlayerSpawnPoint();
        groundPos_ = glm::vec3(spawnPos.x, ArrivalGroundY(spawnPos, spawnPos.y), spawnPos.z);

        // ショットは(横, カメラ直下の地面からの高さ, 前)で持っている
        const auto shotPos = [this](const glm::vec3& shot)
        {
            glm::vec3 pos = groundPos_ + side_ * shot.x + forward_ * shot.z;
            pos.y = ArrivalGroundY(pos, groundPos_.y) + shot.y;
            return pos;
        };
        cameraStartPos_ = shotPos(context->ArrivalCameraStart());
        cameraEndPos_   = shotPos(context->ArrivalCameraEnd  ());

        if (context->HasArrivalPortalPrefab())
        {
            portal_ = NanamiEngine::Scene::GameObject::Instantiate(context->ArrivalPortalPrefab(), PortalCenter(), facingRot);
            if (const auto portal = portal_.lock())
            {
                portalScale_ = portal->Transform().GetLocalScale();
                portal->Transform().SetLocalScale(portalScale_ * ARRIVAL_PORTAL_MIN_OPEN_RATE);
            }
        }

        // 開く前のポータル越しに見えてしまうので、歩き出すまでは膜の奥で消しておく
        avatar->GetEventSceneStateMachine().OnDisable();
        avatar->PlayerTransform().SetWorldPos(WalkPos(0.0f));
        avatar->PlayerTransform().SetWorldRot(facingRot);
        SetAvatarVisible(false);

        if (const auto camera = context->ArrivalCamera())
        {
            cameraFollow_ = camera->Components().Catch<CineMachine::Behaviour::VirtualCameraFollowBehaviour>();
            cameraLookAt_ = camera->Components().Catch<CineMachine::Behaviour::VirtualCameraLookAtBehaviour>();
            camera->SetPriority(ARRIVAL_CAMERA_PRIORITY);

            const auto follow = cameraFollow_.lock();
            const auto lookAt = cameraLookAt_.lock();
            if (follow && lookAt)
            {
                // Follow/LookAtのtargetはどちらもスポーン地点のマーカーなので、そこからのオフセットで置く
                follow->followOffset_ = cameraStartPos_ - spawnPos;
                lookAt->SetOffsetPos(PortalCenter() - spawnPos);

                // Follow/LookAtが次に動くまでカメラは古い姿勢のままなので、先に合わせてからBrainをスナップさせる。
                // スナップしないと、シーンに置かれたBrainの初期位置(スポーン地点から約1700離れている)から補間で飛んでくる
                camera->Transform().SetWorldPos(cameraStartPos_);
                lookAt->LookAtTarget();
                if (const auto brain = context->CameraBrain())
                    brain->SnapToVirtualCamera(*camera);
            }
        }

        isBegun_ = true;
    }

    Coroutine::Task<void> GrassLandArrivalMovie::PlayAsync(std::shared_ptr<GrassLandArrivalMovie> self)
    {
        // ChangeMainSceneがSkipNextFrameを60回積んでいる間はDeltaTimeが0で、コルーチンごと凍る
        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (self->isCanceled_ || !self->isBegun_)
            co_return;

        const auto context = self->context_.lock();
        if (!context)
            co_return;

        const int openDelay_msecs  = ArrivalDuring_msecs(context->ArrivalPortalOpenDelay_msecs ());
        const int open_msecs       = ArrivalDuring_msecs(context->ArrivalPortalOpen_msecs      ());
        const int walk_msecs       = ArrivalDuring_msecs(context->ArrivalWalk_msecs            ());
        const int closeDelay_msecs = ArrivalDuring_msecs(context->ArrivalPortalCloseDelay_msecs());
        const int close_msecs      = ArrivalDuring_msecs(context->ArrivalPortalClose_msecs     ());
        const int hold_msecs       = (std::max)(context->ArrivalHold_msecs(), 0);
        const int walkStart_msecs  = openDelay_msecs + open_msecs;
        const glm::vec3 lookAtHeight(0.0f, context->ArrivalLookAtHeight(), 0.0f);
        const glm::vec3 anchor = context->PlayerSpawnPoint();

        // 勢いよく開いて一度行き過ぎ、歩き出してしばらくしたら一度膨らんでから閉じる
        const glm::vec3 closedScale = self->portalScale_ * ARRIVAL_PORTAL_MIN_OPEN_RATE;
        auto portalScale = tweeny::from(closedScale)
            .to(closedScale       ).during(openDelay_msecs )
            .to(self->portalScale_).during(open_msecs      ).via(Tween::Ease(EaseType::OutBack))
            .to(self->portalScale_).during(closeDelay_msecs)
            .to(closedScale       ).during(close_msecs     ).via(Tween::Ease(EaseType::InBack));

        const glm::vec3 walkFrom = self->WalkPos(0.0f);
        auto walk = tweeny::from(walkFrom)
            .to(walkFrom           ).during(walkStart_msecs)
            .to(self->WalkPos(1.0f)).during(walk_msecs     ).via(Tween::Ease(EaseType::Linear));

        auto cameraOffset = tweeny::from(self->cameraStartPos_ - anchor)
            .to(self->cameraEndPos_ - anchor).during(walkStart_msecs + walk_msecs).via(Tween::Ease(EaseType::InOutSine));

        const std::uint32_t end_msecs = (std::max)(walk.duration() + static_cast<std::uint32_t>(hold_msecs), portalScale.duration());

        float         elapsed_secs  = 0.0f;
        std::uint32_t elapsed_msecs = 0;
        bool isWalking   = false;
        bool isSkipArmed = false;
        while (elapsed_msecs < end_msecs)
        {
            co_await Coroutine::WaitYield();
            if (self->isCanceled_)
                co_return;

            const auto avatar = self->playerAvatar_.lock();
            if (!avatar)
                break;

            // 秒で積んでからミリ秒へ直す。フレームごとにミリ秒へ切り捨てて積むと、その分だけ演出が遅れていく
            elapsed_secs += Time::DeltaTime();
            elapsed_msecs = static_cast<std::uint32_t>(elapsed_secs * 1000.0f);

            if (elapsed_msecs >= portalScale.duration())
                self->DestroyPortal();
            else if (const auto portal = self->portal_.lock())
                portal->Transform().SetLocalScale(portalScale.seek(elapsed_msecs));

            // 開ききった時点では膜の奥にいるので、ここで出しても膜に隠れて見えない
            if (!isWalking && elapsed_msecs >= static_cast<std::uint32_t>(walkStart_msecs))
            {
                isWalking = true;
                self->SetAvatarVisible(true);
                avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::WarpIn);

                // 注視点をポータルからプレイヤーへ付け替える。振り向きはBrainの回転補間に任せる
                if (const auto lookAt = self->cameraLookAt_.lock())
                {
                    lookAt->SetTarget(avatar->PlayerTransform().GetGameObject());
                    lookAt->SetOffsetPos(lookAtHeight);
                }
            }

            if (isWalking && !self->isWalkFinished_)
            {
                glm::vec3 pos = walk.seek(elapsed_msecs);
                pos.y = ArrivalGroundY(pos, self->groundPos_.y);
                avatar->PlayerTransform().SetWorldPos(pos);
                if (walk.isFinished())
                {
                    self->isWalkFinished_ = true;
                    avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
                }
            }

            if (const auto follow = self->cameraFollow_.lock())
                follow->followOffset_ = cameraOffset.seek(elapsed_msecs);

            // ステージ選択の決定キーを押しっぱなしで来ても即スキップにならないよう、一度離すまで待つ
            const bool isDown = ArrivalIsSkipInputDown();
            isSkipArmed |= !isDown;
            if (isSkipArmed && isDown)
                break;
        }

        self->Finish();
    }

    glm::vec3 GrassLandArrivalMovie::WalkPos(const float rate) const
    {
        const auto context = context_.lock();
        if (!context)
            return groundPos_;

        glm::vec3 pos = groundPos_ + forward_ * (context->ArrivalWalkDistance() * rate - context->ArrivalWalkStartBehind());
        pos.y = ArrivalGroundY(pos, groundPos_.y);
        return pos;
    }

    glm::vec3 GrassLandArrivalMovie::PortalCenter() const
    {
        const auto context = context_.lock();
        return groundPos_ + glm::vec3(0.0f, context ? context->ArrivalPortalHeight() : 0.0f, 0.0f);
    }

    void GrassLandArrivalMovie::DestroyPortal()
    {
        if (const auto portal = portal_.lock())
            portal->OnDestroy();
        portal_.reset();
    }

    void GrassLandArrivalMovie::SetAvatarVisible(const bool isVisible) const
    {
        const auto avatar = playerAvatar_.lock();
        if (!avatar)
            return;

        const auto avatarObject = avatar->PlayerTransform().GetGameObject();
        if (!avatarObject)
            return;

        if (const auto renderer = avatarObject->Components().Catch<NanamiEngine::Module::Component::ModelRenderer>().lock())
            renderer->SetEnable(isVisible);
    }

    void GrassLandArrivalMovie::Finish()
    {
        if (isFinished_)
            return;
        isFinished_ = true;

        DestroyPortal();
        SetAvatarVisible(true);

        // 優先度を戻すとFollowFromBehind(0)が勝ち、Brainのブレンドで三人称へ帰る
        if (const auto context = context_.lock())
        {
            if (const auto camera = context->ArrivalCamera())
                camera->OnDisable();
        }

        // 歩き終えていればもうIdleで立ち止まっている。途中で打ち切ったときだけ、立ち止まる位置へ送ってから操作を返す
        if (isWalkFinished_)
            return;
        isWalkFinished_ = true;

        if (const auto avatar = playerAvatar_.lock())
        {
            avatar->PlayerTransform().SetWorldPos(WalkPos(1.0f));
            avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
        }
    }
}
