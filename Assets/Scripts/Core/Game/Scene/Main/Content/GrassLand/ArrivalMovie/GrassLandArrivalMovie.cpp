#include "GrassLandArrivalMovie.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "Engine/Core/Platform/Input/Input.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "Libs/tweeny/Tweeny/tweeny.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../PlayerAvatar/StateMachine/EventScene/PlayerAvatarEventSceneStateType.h"
#include "../../../../../Story/Story_StoryProgress.h"
#include "../../../../../../../GamePlay/Ui/StageArrival/Ui_StageArrivalCaption.h"
#include "../Context/GrassLandSceneContext.h"
#include "../../DrySand/Context/DrySandSceneContext.h"
#include "../../DragonNest/Context/DragonNestSceneContext.h"

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
            if (NanamiEngine::Platform::Input::Keyboard::IsAnyDown())
                return true;

            const auto xInput = NanamiEngine::Platform::Input::Gamepad::Get();
            if (!xInput.connected)
                return false;

            if (xInput.leftTrigger > ARRIVAL_SKIP_TRIGGER_DEAD_ZONE || xInput.rightTrigger > ARRIVAL_SKIP_TRIGGER_DEAD_ZONE)
                return true;

            for (const bool button : xInput.buttons)
            {
                if (button)
                    return true;
            }
            return false;
        }
    }

    template<class TContext>
    StageArrivalMovie<TContext>::StageArrivalMovie(
          const std::weak_ptr<IPlayerAvatar>& playerAvatar
        , const std::shared_ptr<TContext>& context
        , const std::optional<Story::StoryFlag> overviewSeenFlag)
        : playerAvatar_    (playerAvatar    )
        , context_         (context         )
        , overviewSeenFlag_(overviewSeenFlag)
    {
    }

    template<class TContext>
    void StageArrivalMovie<TContext>::Begin()
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
        // NOTE: 空撮は初めて着いたときだけ。見たことは各ピアの物語の進み具合に残す
        const bool isOverviewSeen = overviewSeenFlag_ && Story::StoryProgress::Instance().IsSet(*overviewSeenFlag_);
        hasOverview_    = context->ArrivalOverview_msecs() > 0 && !isOverviewSeen;

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
        controlLock_.Set(NanamiEngine::ControlLock::Service::Instance().Acquire());
        avatar->PlayerTransform().SetWorldPos(WalkPos(0.0f));
        avatar->PlayerTransform().SetWorldRot(facingRot);
        SetAvatarVisible(false);

        if (const auto camera = context->ArrivalCamera())
        {
            cameraFollow_ = camera->Components().Catch<CineMachine::Behaviour::VirtualCameraFollowBehaviour>();
            cameraLookAt_ = camera->Components().Catch<CineMachine::Behaviour::VirtualCameraLookAtBehaviour>();
            camera->SetPriority(ARRIVAL_CAMERA_PRIORITY);

            // スナップしないと、シーンに置かれたBrainの初期位置(スポーン地点から約1700離れている)から補間で飛んでくる
            SnapCamera(
                hasOverview_ ? context->ArrivalOverviewCameraStart() : cameraStartPos_,
                hasOverview_ ? context->ArrivalOverviewLookAt()      : PortalCenter());
        }

        if (hasOverview_)
        {
            if (const auto prefab = context->ArrivalCaptionPrefab())
            {
                if (const auto captionObject = NanamiEngine::Scene::GameObject::Instantiate(*prefab).lock())
                    caption_ = captionObject->Components().Catch<GamePlay::Ui::StageArrivalCaption>();
            }
        }

        isBegun_ = true;
    }

    template<class TContext>
    Coroutine::Task<void> StageArrivalMovie<TContext>::PlayAsync(std::shared_ptr<StageArrivalMovie> self)
    {
        // ChangeMainSceneがSkipNextFrameを60回積んでいる間はDeltaTimeが0で、コルーチンごと凍る
        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (self->isCanceled_ || !self->isBegun_)
            co_return;

        // NOTE: スキップしても見たことにする。途中でシーンを抜けたときは次に来たときにもう一度流す
        if (co_await PlayOverviewAsync(self))
        {
            self->MarkOverviewSeen();
            self->Finish();
            co_return;
        }
        if (self->isCanceled_)
            co_return;
        if (self->hasOverview_)
        {
            self->MarkOverviewSeen();
            self->ReleaseCaption();
        }

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
        auto portalScaleTween = tweeny::from(closedScale)
            .to(closedScale       ).during(openDelay_msecs )
            .to(self->portalScale_).during(open_msecs      ).via(Tween::Ease(EaseType::OutBack))
            .to(self->portalScale_).during(closeDelay_msecs)
            .to(closedScale       ).during(close_msecs     ).via(Tween::Ease(EaseType::InBack));

        const glm::vec3 walkFrom = self->WalkPos(0.0f);
        auto walkTween = tweeny::from(walkFrom)
            .to(walkFrom           ).during(walkStart_msecs)
            .to(self->WalkPos(1.0f)).during(walk_msecs     ).via(Tween::Ease(EaseType::Linear));

        auto cameraOffsetTween = tweeny::from(self->cameraStartPos_ - anchor)
            .to(self->cameraEndPos_ - anchor).during(walkStart_msecs + walk_msecs).via(Tween::Ease(EaseType::InOutSine));

        const std::uint32_t end_msecs = (std::max)(walkTween.duration() + static_cast<std::uint32_t>(hold_msecs), portalScaleTween.duration());

        LibCore::Tween::TweenPlayer<glm::vec3> portalScale;
        LibCore::Tween::TweenPlayer<glm::vec3> walk;
        LibCore::Tween::TweenPlayer<glm::vec3> cameraOffset;
        portalScale .Play(std::move(portalScaleTween ));
        walk        .Play(std::move(walkTween        ));
        cameraOffset.Play(std::move(cameraOffsetTween));

        float elapsed_secs = 0.0f;
        bool isWalking = false;
        while (elapsed_secs * 1000.0f < static_cast<float>(end_msecs))
        {
            co_await Coroutine::WaitYield();
            if (self->isCanceled_)
                co_return;

            const auto avatar = self->playerAvatar_.lock();
            if (!avatar)
                break;

            const float deltaTime = Time::DeltaTime();
            elapsed_secs += deltaTime;
            portalScale .Tick(deltaTime);
            walk        .Tick(deltaTime);
            cameraOffset.Tick(deltaTime);

            if (portalScale.IsFinished())
                self->DestroyPortal();
            else if (const auto portal = self->portal_.lock())
                portal->Transform().SetLocalScale(portalScale.Value());

            // 開ききった時点では膜の奥にいるので、ここで出しても膜に隠れて見えない
            if (!isWalking && elapsed_secs * 1000.0f >= static_cast<float>(walkStart_msecs))
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
                glm::vec3 pos = walk.Value();
                pos.y = ArrivalGroundY(pos, self->groundPos_.y);
                avatar->PlayerTransform().SetWorldPos(pos);
                if (walk.IsFinished())
                {
                    self->isWalkFinished_ = true;
                    self->controlLock_.Release();
                    avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
                }
            }

            if (const auto follow = self->cameraFollow_.lock())
                follow->followOffset_ = cameraOffset.Value();

            if (self->IsSkipRequested())
                break;
        }

        self->Finish();
    }

    template<class TContext>
    Coroutine::Task<bool> StageArrivalMovie<TContext>::PlayOverviewAsync(std::shared_ptr<StageArrivalMovie> self)
    {
        const auto context = self->context_.lock();
        if (!context || !self->hasOverview_)
            co_return false;

        const glm::vec3 overviewEnd    = context->ArrivalOverviewCameraEnd();
        const glm::vec3 overviewLookAt = context->ArrivalOverviewLookAt();

        // 島の名前を出しながら島を見下ろす
        if (const auto caption = self->caption_.lock(); caption && !context->ArrivalIslandTitle().empty())
            caption->ShowIsland(context->ArrivalIslandTitle(), context->ArrivalIslandSubtitle());
        if (co_await MoveCameraAsync(self, { context->ArrivalOverviewCameraStart(), overviewEnd,
                                             overviewLookAt, overviewLookAt, context->ArrivalOverview_msecs() }))
            co_return true;
        if (self->isCanceled_)
            co_return false;

        // 見どころを1か所ずつ切り替えで映す
        glm::vec3 descendFrom       = overviewEnd;
        glm::vec3 descendLookAtFrom = overviewLookAt;
        for (const auto& shot : context->ArrivalTourShots())
        {
            self->SnapCamera(shot.cameraStart, shot.lookAt);
            if (const auto caption = self->caption_.lock(); caption && !shot.title.empty())
                caption->ShowLandmark(shot.title, shot.subtitle);
            if (co_await MoveCameraAsync(self, { shot.cameraStart, shot.cameraEnd, shot.lookAt, shot.lookAt, shot.duration_msecs }))
                co_return true;
            if (self->isCanceled_)
                co_return false;
            descendFrom       = shot.cameraEnd;
            descendLookAtFrom = shot.lookAt;
        }

        // NOTE: 最後の見どころからそのままポータルへ降りる。見どころはポータルに近いものを最後に並べる
        co_return co_await MoveCameraAsync(self, { descendFrom, self->cameraStartPos_, descendLookAtFrom, self->PortalCenter(),
                                                   context->ArrivalOverviewDescend_msecs(), true });
    }

    template<class TContext>
    Coroutine::Task<bool> StageArrivalMovie<TContext>::MoveCameraAsync(std::shared_ptr<StageArrivalMovie> self, const CameraMove move)
    {
        const auto context = self->context_.lock();
        if (!context)
            co_return false;

        const int duration_msecs = ArrivalDuring_msecs(move.duration_msecs);
        const glm::vec3 anchor = context->PlayerSpawnPoint();

        auto horizontalTween = tweeny::from(move.from)
            .to(move.to).during(duration_msecs).via(Tween::Ease(EaseType::InOutSine));
        auto heightTween = tweeny::from(move.from.y)
            .to(move.to.y).during(duration_msecs).via(Tween::Ease(move.lagsHeight ? EaseType::InOutCubic : EaseType::InOutSine));
        auto lookAtTween = tweeny::from(move.lookFrom)
            .to(move.lookTo).during(duration_msecs).via(Tween::Ease(EaseType::InOutSine));

        LibCore::Tween::TweenPlayer<glm::vec3> horizontal;
        LibCore::Tween::TweenPlayer<float>     height;
        LibCore::Tween::TweenPlayer<glm::vec3> lookAtPos;
        horizontal.Play(std::move(horizontalTween));
        height    .Play(std::move(heightTween    ));
        lookAtPos .Play(std::move(lookAtTween    ));

        float elapsed_secs = 0.0f;
        while (elapsed_secs * 1000.0f < static_cast<float>(duration_msecs))
        {
            co_await Coroutine::WaitYield();
            if (self->isCanceled_)
                co_return false;

            const float deltaTime = Time::DeltaTime();
            elapsed_secs += deltaTime;
            horizontal.Tick(deltaTime);
            height    .Tick(deltaTime);
            lookAtPos .Tick(deltaTime);

            if (const auto follow = self->cameraFollow_.lock())
            {
                const glm::vec3 pos(horizontal.Value().x, height.Value(), horizontal.Value().z);
                follow->followOffset_ = pos - anchor;
            }
            if (const auto lookAt = self->cameraLookAt_.lock())
                lookAt->SetOffsetPos(lookAtPos.Value() - anchor);

            // 字幕は次のショットへ切り替わる前に消しきる
            if (const auto caption = self->caption_.lock())
            {
                const float remaining_secs = static_cast<float>(duration_msecs) / 1000.0f - elapsed_secs;
                if (remaining_secs <= caption->FadeOut_secs())
                    caption->Hide();
            }

            if (self->IsSkipRequested())
                co_return true;
        }
        co_return false;
    }

    template<class TContext>
    void StageArrivalMovie<TContext>::SnapCamera(const glm::vec3& pos, const glm::vec3& lookAtPos) const
    {
        const auto context = context_.lock();
        const auto follow  = cameraFollow_.lock();
        const auto lookAt  = cameraLookAt_.lock();
        if (!context || !follow || !lookAt)
            return;

        const auto camera = context->ArrivalCamera();
        if (!camera)
            return;

        // Follow/LookAtのtargetはどちらもスポーン地点のマーカーなので、そこからのオフセットで置く
        const glm::vec3 anchor = context->PlayerSpawnPoint();
        follow->followOffset_ = pos - anchor;
        lookAt->SetOffsetPos(lookAtPos - anchor);

        // Follow/LookAtが次に動くまでカメラは古い姿勢のままなので、先に合わせてからBrainをスナップさせる
        camera->Transform().SetWorldPos(pos);
        lookAt->LookAtTarget();
        if (const auto brain = context->CameraBrain())
            brain->SnapToVirtualCamera(*camera);
    }

    template<class TContext>
    void StageArrivalMovie<TContext>::MarkOverviewSeen() const
    {
        if (overviewSeenFlag_)
            Story::StoryProgress::Instance().Set(*overviewSeenFlag_);
    }

    template<class TContext>
    void StageArrivalMovie<TContext>::ReleaseCaption()
    {
        if (const auto caption = caption_.lock())
            caption->HideAndDestroy();
        caption_.reset();
    }

    template<class TContext>
    bool StageArrivalMovie<TContext>::IsSkipRequested()
    {
        // ステージ選択の決定キーを押しっぱなしで来ても即スキップにならないよう、一度離すまで待つ
        const bool isDown = ArrivalIsSkipInputDown();
        isSkipArmed_ |= !isDown;
        return isSkipArmed_ && isDown;
    }

    template<class TContext>
    glm::vec3 StageArrivalMovie<TContext>::WalkPos(const float rate) const
    {
        const auto context = context_.lock();
        if (!context)
            return groundPos_;

        glm::vec3 pos = groundPos_ + forward_ * (context->ArrivalWalkDistance() * rate - context->ArrivalWalkStartBehind());
        pos.y = ArrivalGroundY(pos, groundPos_.y);
        return pos;
    }

    template<class TContext>
    glm::vec3 StageArrivalMovie<TContext>::PortalCenter() const
    {
        const auto context = context_.lock();
        return groundPos_ + glm::vec3(0.0f, context ? context->ArrivalPortalHeight() : 0.0f, 0.0f);
    }

    template<class TContext>
    void StageArrivalMovie<TContext>::DestroyPortal()
    {
        if (const auto portal = portal_.lock())
            portal->OnDestroy();
        portal_.reset();
    }

    template<class TContext>
    void StageArrivalMovie<TContext>::SetAvatarVisible(const bool isVisible) const
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

    template<class TContext>
    void StageArrivalMovie<TContext>::Finish()
    {
        if (isFinished_)
            return;
        isFinished_ = true;

        DestroyPortal();
        ReleaseCaption();
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
        controlLock_.Release();

        if (const auto avatar = playerAvatar_.lock())
        {
            avatar->PlayerTransform().SetWorldPos(WalkPos(1.0f));
            avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
        }
    }

    template class StageArrivalMovie<GrassLandSceneContext>;
    template class StageArrivalMovie<DrySandSceneContext>;
    template class StageArrivalMovie<DragonNestSceneContext>;
}
