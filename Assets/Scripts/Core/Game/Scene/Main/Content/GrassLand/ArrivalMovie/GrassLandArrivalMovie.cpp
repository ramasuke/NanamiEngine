#include "GrassLandArrivalMovie.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"

#include "../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../../../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../PlayerAvatar/SwordMan/State/SwordManAvatarStateType.h"
#include "../Context/GrassLandSceneContext.h"

namespace GameCore::Scene::GrassLand
{
    namespace
    {
        // 演出中だけプレイヤーのFollowFromBehind(0)より上に出す
        constexpr int ARRIVAL_CAMERA_PRIORITY = 100;
        constexpr unsigned char XINPUT_TRIGGER_DEAD_ZONE = 30;
        constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;

        float ArrivalSmoothstep(const float rate) { return rate * rate * (3.0f - 2.0f * rate); }

        glm::vec3 ArrivalLerp(const glm::vec3& from, const glm::vec3& to, const float rate) { return from + (to - from) * rate; }

        bool IsSkipInputDown()
        {
            if (CheckHitKeyAll(DX_CHECKINPUT_KEY) != 0)
                return true;

            XINPUT_STATE xInput = {};
            if (GetJoypadXInputState(DX_INPUT_PAD1, &xInput) != 0)
                return false;

            if (xInput.LeftTrigger > XINPUT_TRIGGER_DEAD_ZONE || xInput.RightTrigger > XINPUT_TRIGGER_DEAD_ZONE)
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
        if (!context)
            return;

        if (context->HasArrivalPortalPrefab())
            portal_ = NanamiEngine::Scene::GameObject::Instantiate(context->ArrivalPortalPrefab(), context->PlayerSpawnPoint());

        ApplyShot(0.0f);

        if (const auto camera = context->ArrivalCamera())
        {
            camera->SetPriority(ARRIVAL_CAMERA_PRIORITY);
            // スナップしないと、前のシーンのまま残っているBrainの位置から補間で飛んでくる
            if (const auto brain = context->CameraBrain())
                brain->ApplyVirtualCameraMatrix(*camera);
        }

        if (const auto avatar = playerAvatar_.lock())
            avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::SwordMan::SwordManAvatarStateType::WarpIn);
    }

    Coroutine::Task<void> GrassLandArrivalMovie::PlayAsync(std::shared_ptr<GrassLandArrivalMovie> self)
    {
        // ChangeMainSceneがSkipNextFrameを60回積んでいる間はDeltaTimeが0で、コルーチンごと凍る
        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (self->isCanceled_)
            co_return;

        const auto context = self->context_.lock();
        const float during_secs = context ? static_cast<float>(context->ArrivalShotDuring_msecs()) / 1000.0f : 0.0f;

        float elapsed_secs = 0.0f;
        bool isSkipped     = false;
        bool isSkipArmed   = false;
        while (during_secs > 0.0f && elapsed_secs < during_secs)
        {
            co_await Coroutine::WaitYield();
            if (self->isCanceled_)
                co_return;

            elapsed_secs += Time::DeltaTime();
            self->ApplyShot(ArrivalSmoothstep(std::clamp(elapsed_secs / during_secs, 0.0f, 1.0f)));

            // ステージ選択の決定キーを押しっぱなしで来ても即スキップにならないよう、一度離すまで待つ
            const bool isDown = IsSkipInputDown();
            isSkipArmed |= !isDown;
            if (isSkipArmed && isDown)
            {
                isSkipped = true;
                break;
            }
        }

        self->Finish(isSkipped);
    }

    void GrassLandArrivalMovie::ApplyShot(const float rate) const
    {
        const auto context = context_.lock();
        if (!context)
            return;

        const auto camera = context->ArrivalCamera();
        if (!camera)
            return;

        const auto follow = camera->Components().Catch<CineMachine::Behaviour::VirtualCameraFollowBehaviour>().lock();
        const auto lookAt = camera->Components().Catch<CineMachine::Behaviour::VirtualCameraLookAtBehaviour>().lock();
        if (!follow || !lookAt)
            return;

        // (水平角[deg], 仰俯角[deg], 距離)。水平角0がプレイヤーの背面(+Z)
        const glm::vec3 shot = ArrivalLerp(context->ArrivalShotStart(), context->ArrivalShotEnd(), rate);
        const float yaw      = shot.x * DEG_TO_RAD;
        const float pitch    = shot.y * DEG_TO_RAD;
        const float distance = shot.z;

        follow->followOffset_ = glm::vec3(
            distance * std::cos(pitch) * std::sin(yaw),
            distance * std::sin(pitch),
            distance * std::cos(pitch) * std::cos(yaw));
        lookAt->SetOffsetPos(ArrivalLerp(context->ArrivalLookAtOffsetStart(), context->ArrivalLookAtOffsetEnd(), rate));
    }

    void GrassLandArrivalMovie::Finish(const bool isSkipped)
    {
        if (isFinished_)
            return;
        isFinished_ = true;

        if (const auto portal = portal_.lock())
            portal->OnDestroy();
        portal_.reset();

        // 優先度を戻すとFollowFromBehind(0)が勝ち、Brainのブレンドで三人称へ帰る
        if (const auto context = context_.lock())
        {
            if (const auto camera = context->ArrivalCamera())
                camera->OnDisable();
        }

        // 完走ならStateが自分でIdleへ抜けている。スキップのときだけ、せり上がりの途中でも打ち切る
        if (!isSkipped)
            return;

        if (const auto avatar = playerAvatar_.lock())
            avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::SwordMan::SwordManAvatarStateType::Idle);
    }
}
