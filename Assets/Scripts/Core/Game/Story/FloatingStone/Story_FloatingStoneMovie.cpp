#include "Story_FloatingStoneMovie.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "DxLib.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Core/Physics/Physics.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "../../PlayerAvatar/IPlayerAvatar.h"

namespace GameCore::Story::FloatingStone
{
    namespace
    {
        // 到着演出(100)や NPC の会話カメラより上に出す
        constexpr int STONE_CAMERA_PRIORITY = 110;
        constexpr unsigned char STONE_SKIP_TRIGGER_DEAD_ZONE = 30;
        // 大顎を倒した直後は攻撃ボタンを連打しているので、始まってしばらくはスキップを受け付けない
        constexpr float STONE_SKIP_GRACE_SECS = 1.5f;
        // GreenCoreShard.mv1 の結晶の中ほど(モデルの単位)。LookAt と光の尾はここに合わせる
        constexpr float STONE_CENTER_HEIGHT = 8.5f;

        // 草原: 大顎が倒れきるのを待つ → 震える → 抜け出して浮き上がる → 空へ飛び去る → 見送る
        constexpr float DEPART_DELAY_SECS  = 2.5f;
        constexpr float DEPART_SHAKE_SECS  = 1.6f;
        constexpr float DEPART_RISE_SECS   = 2.6f;
        constexpr float DEPART_FLY_SECS    = 2.4f;
        constexpr float DEPART_HOLD_SECS   = 0.8f;
        constexpr float DEPART_SHAKE_WIDTH = 1.2f;
        constexpr float DEPART_RISE_HEIGHT = 70.0f;
        constexpr float DEPART_RISE_TURN_DEGREES = 120.0f;
        constexpr float DEPART_FLY_TURN_DEGREES  = 540.0f;
        // 拠点の島の方角(空の高いところ)へ抜けていく
        const glm::vec3 DEPART_FLY_OFFSET(-500.0f, 900.0f, -700.0f);

        // 拠点の島: 読み込みが明けるのを待つ → 遠くから飛んでくる → 底の真下で減速してはまる → 見届ける
        constexpr float RETURN_DELAY_SECS  = 1.0f;
        constexpr float RETURN_FLY_SECS    = 4.0f;
        constexpr float RETURN_SETTLE_SECS = 1.4f;
        constexpr float RETURN_HOLD_SECS   = 2.2f;
        constexpr float RETURN_FLY_TURN_DEGREES = 540.0f;
        const glm::vec3 RETURN_START_OFFSET   (900.0f, -350.0f, 900.0f);
        const glm::vec3 RETURN_APPROACH_OFFSET(0.0f, -90.0f, 0.0f);

        // 戻る前の島と階段は、雲のずっと下に退避しておく(隠してもコライダーは当たり続けるため)
        const glm::vec3 ISLAND_SUNK_OFFSET(0.0f, -3000.0f, 0.0f);
        // 草原の後の島: 読み込みが明けるのを待つ → 雲の下からせり上がる → 階段が手前から1段ずつ架かる → 見届ける
        constexpr float ISLAND_DELAY_SECS        = 0.6f;
        constexpr float ISLAND_RISE_SECS         = 6.0f;
        constexpr float ISLAND_RISE_DEPTH        = 900.0f;
        constexpr float ISLAND_RISE_TILT_DEGREES = 7.0f;
        constexpr float STAIRS_DELAY_SECS        = 0.8f;
        constexpr float STAIRS_STEP_SECS         = 0.7f;
        constexpr float STAIRS_INTERVAL_SECS     = 0.45f;
        constexpr float STAIRS_STEP_DROP         = 40.0f;
        constexpr float ISLAND_HOLD_SECS         = 1.8f;

        float EaseOutCubic (const float t) { return 1.0f - std::pow(1.0f - t, 3.0f); }
        float EaseInCubic  (const float t) { return t * t * t; }
        float EaseInOutSine(const float t) { return 0.5f - 0.5f * std::cos(t * 3.14159265f); }
        float EaseOutBack  (const float t) { const float u = t - 1.0f; return 1.0f + 2.70158f * u * u * u + 1.70158f * u * u; }
        float Rate(const float elapsed_secs, const float during_secs) { return std::clamp(elapsed_secs / during_secs, 0.0f, 1.0f); }

        glm::quat Yaw(const float degrees) { return glm::angleAxis(glm::radians(degrees), glm::vec3(0.0f, 1.0f, 0.0f)); }

        bool IsSkipInputDown()
        {
            if (CheckHitKeyAll(DX_CHECKINPUT_KEY) != 0)
                return true;

            XINPUT_STATE xInput = {};
            if (GetJoypadXInputState(DX_INPUT_PAD1, &xInput) != 0)
                return false;

            if (xInput.LeftTrigger > STONE_SKIP_TRIGGER_DEAD_ZONE || xInput.RightTrigger > STONE_SKIP_TRIGGER_DEAD_ZONE)
                return true;

            for (const auto button : xInput.Buttons)
            {
                if (button != 0)
                    return true;
            }
            return false;
        }

        /** @brief 押しっぱなしで入ってきても即スキップにならないよう、一度離すまで待つ */
        class SkipInput final
        {
        public:
            bool IsSkipped()
            {
                const bool isDown = IsSkipInputDown();
                isArmed_ |= !isDown;
                return isArmed_ && isDown;
            }

        private:
            bool isArmed_ = false;
        };

        void SetStoneParticles(NanamiEngine::Module::GameObject::IGameObject& stone, const bool isPlaying)
        {
            for (const auto& child : stone.Transform().GetAllChildren())
            {
                if (const auto particle = child->Components().Catch<NanamiEngine::Module::Component::ParticleSystem>().lock())
                {
                    if (isPlaying)
                        particle->Play();
                    else
                        particle->Stop();
                }
            }
        }

        glm::vec3 StoneCenter(NanamiEngine::Module::GameObject::IGameObject& stone)
        {
            const auto& transform = stone.Transform();
            return transform.GetWorldPos() + transform.GetWorldRot() * glm::vec3(0.0f, STONE_CENTER_HEIGHT * transform.GetWorldScale().y, 0.0f);
        }

        /** @brief カメラとプレイヤーの操作を演出のあいだだけ借りる */
        class MovieScope final
        {
        public:
            explicit MovieScope(const StoneMovieCast& cast)
                : playerAvatar_(cast.playerAvatar)
                , camera_(cast.camera)
                , lookTarget_(cast.stone)
                , lookOffset_(StoneCenter(*cast.stone) - cast.stone->Transform().GetWorldPos())
                , flightParticle_(cast.flightParticle)
                , burstParticle_(cast.burstParticle)
            {
            }

            explicit MovieScope(const IslandReturnCast& cast)
                : playerAvatar_(cast.playerAvatar)
                , camera_(cast.camera)
                , lookTarget_(cast.focus)
            {
            }

            ~MovieScope() { End(); }

            void Begin()
            {
                if (const auto avatar = playerAvatar_.lock())
                    avatar->GetEventSceneStateMachine().OnDisable();

                if (!camera_)
                    return;

                const auto lookAt = camera_->Components().Catch<CineMachine::Behaviour::VirtualCameraLookAtBehaviour>().lock();
                if (lookAt && lookTarget_)
                {
                    lookAt->SetTarget(lookTarget_);
                    lookAt->SetOffsetPos(lookOffset_);
                }
                camera_->SetPriority(STONE_CAMERA_PRIORITY);
            }

            void SpawnFlight()
            {
                if (flightParticle_)
                    flight_ = NanamiEngine::Scene::GameObject::Instantiate(flightParticle_, StoneCenter(*lookTarget_));
            }

            void MoveFlight() const
            {
                if (const auto flight = flight_.lock())
                    flight->Transform().SetWorldPos(StoneCenter(*lookTarget_));
            }

            void DestroyFlight()
            {
                if (const auto flight = flight_.lock())
                    flight->OnDestroy();
                flight_.reset();
            }

            void Burst(const glm::vec3& pos) const
            {
                if (burstParticle_)
                    NanamiEngine::Scene::GameObject::Instantiate(burstParticle_, pos);
            }

            void End()
            {
                if (isEnded_)
                    return;
                isEnded_ = true;

                DestroyFlight();
                // 優先度を戻すと三人称カメラが勝ち、Brain のブレンドで帰る
                if (camera_)
                    camera_->OnDisable();
                if (const auto avatar = playerAvatar_.lock())
                    avatar->GetEventSceneStateMachine().OnEnable();
            }

        private:
            std::weak_ptr<IPlayerAvatar> playerAvatar_;
            std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera_;
            std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> lookTarget_;
            glm::vec3 lookOffset_ = glm::vec3(0.0f);
            std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile> flightParticle_;
            std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile> burstParticle_;
            std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> flight_;
            bool isEnded_ = false;
        };

        /** @brief 動かした物のコライダーを、次の Flush で今の位置に作り直させる(Static の Body は Transform に付いてこない) */
        void RebuildColliders(NanamiEngine::Module::GameObject::IGameObject& root)
        {
            auto& bodies = NanamiEngine::Core::Application::ApplicationBase::Physics().Bodies();
            const auto rebuild = [&bodies](NanamiEngine::Module::GameObject::IGameObject& gameObject)
            {
                for (const auto& weak : gameObject.Components().Catches<NanamiEngine::Module::Component::ColliderBase>())
                {
                    if (const auto collider = weak.lock())
                        bodies.MarkDirty(*collider);
                }
            };
            rebuild(root);
            for (const auto& child : root.Transform().GetAllChildren())
                rebuild(*child);
        }

        void MoveBy(NanamiEngine::Module::GameObject::IGameObject& gameObject, const glm::vec3& offset)
        {
            gameObject.Transform().SetWorldPos(gameObject.Transform().GetWorldPos() + offset);
        }
    }

    void SetStoneVisible(NanamiEngine::Module::GameObject::IGameObject& stone, const bool isVisible)
    {
        // NOTE: GameObject を無効にしても Effekseer の再生は残るので、パーティクルは先に止める
        if (!isVisible)
            SetStoneParticles(stone, false);
        stone.SetEnable(isVisible);
        if (isVisible)
            SetStoneParticles(stone, true);
    }

    Coroutine::Task<void> PlayDepartAsync(StoneMovieCast cast, std::shared_ptr<bool> isCanceled)
    {
        if (!cast.stone)
            co_return;

        float wait_secs = 0.0f;
        while (wait_secs < DEPART_DELAY_SECS)
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
                co_return;
            wait_secs += Time::DeltaTime();
        }

        auto& transform = cast.stone->Transform();
        const glm::vec3 basePos = transform.GetWorldPos();
        const glm::quat baseRot = transform.GetWorldRot();
        const glm::vec3 risenPos = basePos + glm::vec3(0.0f, DEPART_RISE_HEIGHT, 0.0f);

        MovieScope scope(cast);
        scope.Begin();
        SkipInput skip;

        const float riseStart_secs = DEPART_SHAKE_SECS;
        const float flyStart_secs  = riseStart_secs + DEPART_RISE_SECS;
        const float holdStart_secs = flyStart_secs + DEPART_FLY_SECS;
        const float end_secs       = holdStart_secs + DEPART_HOLD_SECS;

        float elapsed_secs = 0.0f;
        bool isLifted = false;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
                co_return;
            elapsed_secs += Time::DeltaTime();
            if (skip.IsSkipped() && elapsed_secs >= STONE_SKIP_GRACE_SECS)
                break;

            if (elapsed_secs < riseStart_secs)
            {
                // 力が溢れて、だんだん強く震える
                const float width = DEPART_SHAKE_WIDTH * Rate(elapsed_secs, DEPART_SHAKE_SECS);
                const glm::vec3 shake(std::sin(elapsed_secs * 53.0f), 0.0f, std::cos(elapsed_secs * 41.0f));
                transform.SetWorldPos(basePos + shake * width);
                continue;
            }

            if (!isLifted)
            {
                isLifted = true;
                SetStoneParticles(*cast.stone, false);
                scope.Burst(basePos);
                scope.SpawnFlight();
            }

            if (elapsed_secs < flyStart_secs)
            {
                const float t = Rate(elapsed_secs - riseStart_secs, DEPART_RISE_SECS);
                transform.SetWorldPos(glm::mix(basePos, risenPos, EaseOutCubic(t)));
                transform.SetWorldRot(Yaw(DEPART_RISE_TURN_DEGREES * EaseInOutSine(t)) * baseRot);
            }
            else
            {
                const float t = Rate(elapsed_secs - flyStart_secs, DEPART_FLY_SECS);
                transform.SetWorldPos(risenPos + DEPART_FLY_OFFSET * EaseInCubic(t));
                transform.SetWorldRot(Yaw(DEPART_RISE_TURN_DEGREES + DEPART_FLY_TURN_DEGREES * EaseInCubic(t)) * baseRot);
            }
            scope.MoveFlight();
        }

        // 飛び去った石は隠し、置き場所だけ元へ戻しておく
        SetStoneVisible(*cast.stone, false);
        transform.SetWorldPos(basePos);
        transform.SetWorldRot(baseRot);
        scope.End();
    }

    Coroutine::Task<void> PlayReturnAsync(
        StoneMovieCast cast, std::shared_ptr<bool> isCanceled, std::function<bool()> canStart, std::function<void()> onDocked)
    {
        if (!cast.stone)
            co_return;

        // シーン切り替え直後は DeltaTime が 0 で、コルーチンごと凍る
        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (*isCanceled)
            co_return;
        while (canStart && !canStart())
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
                co_return;
        }
        float wait_secs = 0.0f;
        while (wait_secs < RETURN_DELAY_SECS)
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
                co_return;
            wait_secs += Time::DeltaTime();
        }

        auto& transform = cast.stone->Transform();
        const glm::vec3 dockPos     = transform.GetWorldPos();
        const glm::quat dockRot     = transform.GetWorldRot();
        const glm::vec3 startPos    = dockPos + RETURN_START_OFFSET;
        const glm::vec3 approachPos = dockPos + RETURN_APPROACH_OFFSET;

        bool isDocked = false;
        const auto dock = [&]
        {
            if (isDocked)
                return;
            isDocked = true;
            transform.SetWorldPos(dockPos);
            transform.SetWorldRot(dockRot);
            SetStoneParticles(*cast.stone, true);
            if (onDocked)
                onDocked();
        };

        SetStoneVisible(*cast.stone, true);
        SetStoneParticles(*cast.stone, false);
        transform.SetWorldPos(startPos);

        MovieScope scope(cast);
        scope.Begin();
        scope.SpawnFlight();
        SkipInput skip;

        const float settleStart_secs = RETURN_FLY_SECS;
        const float holdStart_secs   = settleStart_secs + RETURN_SETTLE_SECS;
        const float end_secs         = holdStart_secs + RETURN_HOLD_SECS;

        float elapsed_secs = 0.0f;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
            {
                dock();
                co_return;
            }
            elapsed_secs += Time::DeltaTime();
            if (skip.IsSkipped() && elapsed_secs >= STONE_SKIP_GRACE_SECS)
                break;

            if (elapsed_secs < settleStart_secs)
            {
                // 回りながら飛んできて、底の真下で勢いを落とす
                const float t = EaseOutCubic(Rate(elapsed_secs, RETURN_FLY_SECS));
                transform.SetWorldPos(glm::mix(startPos, approachPos, t));
                transform.SetWorldRot(Yaw(RETURN_FLY_TURN_DEGREES * (1.0f - t)) * dockRot);
                scope.MoveFlight();
            }
            else if (elapsed_secs < holdStart_secs)
            {
                const float t = EaseInOutSine(Rate(elapsed_secs - settleStart_secs, RETURN_SETTLE_SECS));
                transform.SetWorldPos(glm::mix(approachPos, dockPos, t));
                transform.SetWorldRot(dockRot);
                scope.MoveFlight();
            }
            else if (!isDocked)
            {
                scope.DestroyFlight();
                scope.Burst(StoneCenter(*cast.stone));
                dock();
            }
        }

        dock();
        scope.End();
    }

    Coroutine::Task<void> PlayScatterAsync(std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stonesRoot, const ScatterShot shot)
    {
        if (!stonesRoot)
            co_return;

        struct Flight
        {
            std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stone;
            glm::vec3 buriedPos;
            glm::vec3 risenPos;
            glm::vec3 direction;
            glm::quat baseRot;
        };
        const glm::vec3 rootPos = stonesRoot->Transform().GetWorldPos();
        std::vector<Flight> flights;
        for (const auto& stone : stonesRoot->Transform().GetChildren())
        {
            const glm::vec3 pos = stone->Transform().GetWorldPos();
            glm::vec3 direction(pos.x - rootPos.x, 0.0f, pos.z - rootPos.z);
            direction = glm::dot(direction, direction) > 0.0001f ? glm::normalize(direction) : glm::vec3(0.0f, 0.0f, 1.0f);
            flights.push_back({ stone, pos, pos + glm::vec3(0.0f, shot.riseHeight, 0.0f), direction, stone->Transform().GetWorldRot() });
            // NOTE: 光の尾(シーンでは PlayMode Manual)は飛び立つまで出さない
            SetStoneParticles(*stone, false);
        }

        const float hoverStart_secs = shot.rise_secs;
        const float flyStart_secs   = hoverStart_secs + shot.hover_secs;
        const float end_secs        = flyStart_secs + shot.fly_secs;

        float elapsed_secs = 0.0f;
        bool isLaunched = false;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            elapsed_secs += Time::DeltaTime();

            if (!isLaunched && elapsed_secs >= flyStart_secs)
            {
                isLaunched = true;
                for (const auto& flight : flights)
                    SetStoneParticles(*flight.stone, true);
            }

            for (const auto& flight : flights)
            {
                auto& transform = flight.stone->Transform();
                if (elapsed_secs < hoverStart_secs)
                {
                    // 地面を割ってせり上がり、回りながら減速する
                    const float t = EaseOutCubic(Rate(elapsed_secs, shot.rise_secs));
                    transform.SetWorldPos(glm::mix(flight.buriedPos, flight.risenPos, t));
                    transform.SetWorldRot(Yaw(180.0f * t) * flight.baseRot);
                }
                else if (elapsed_secs < flyStart_secs)
                {
                    const float hover_secs = elapsed_secs - hoverStart_secs;
                    transform.SetWorldPos(flight.risenPos + glm::vec3(0.0f, std::sin(hover_secs * 6.0f) * 2.0f, 0.0f));
                    transform.SetWorldRot(Yaw(180.0f + 40.0f * hover_secs) * flight.baseRot);
                }
                else
                {
                    // 水平は加速しながら、高さは一定の速さで上がるので、弧を描いて空へ抜ける
                    const float rate = Rate(elapsed_secs - flyStart_secs, shot.fly_secs);
                    transform.SetWorldPos(flight.risenPos
                        + flight.direction * (shot.flyDistance * EaseInCubic(rate))
                        + glm::vec3(0.0f, shot.flyRise * rate, 0.0f));
                    transform.SetWorldRot(Yaw(180.0f + 40.0f * shot.hover_secs + 720.0f * rate) * flight.baseRot);
                }
            }
        }

        for (const auto& flight : flights)
            SetStoneVisible(*flight.stone, false);
    }

    void SinkIsland(const IslandReturnCast& cast)
    {
        for (const auto& target : { cast.island, cast.stairs })
        {
            if (!target)
                continue;

            target->SetEnable(false);
            MoveBy(*target, ISLAND_SUNK_OFFSET);
            RebuildColliders(*target);
        }
    }

    void ShowIsland(const IslandReturnCast& cast)
    {
        for (const auto& target : { cast.island, cast.stairs })
        {
            if (target)
                target->SetEnable(true);
        }
    }

    Coroutine::Task<void> PlayIslandReturnAsync(
        IslandReturnCast cast, std::shared_ptr<bool> isCanceled, std::function<bool()> canStart, std::function<void()> onReturned)
    {
        if (!cast.island || !cast.stairs)
            co_return;

        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (*isCanceled)
            co_return;
        while (canStart && !canStart())
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
                co_return;
        }
        float wait_secs = 0.0f;
        while (wait_secs < ISLAND_DELAY_SECS)
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
                co_return;
            wait_secs += Time::DeltaTime();
        }

        // NOTE: SinkIsland で下ろしてあるので、シーン上の位置(戻った位置)はその分だけ上
        auto& islandTransform = cast.island->Transform();
        const glm::vec3 islandHomePos = islandTransform.GetWorldPos() - ISLAND_SUNK_OFFSET;
        const glm::quat islandHomeRot = islandTransform.GetWorldRot();
        // 傾きは島の見える所(カメラが見る所)を中心にかける。島の原点はモデルの外にある
        const glm::vec3 pivotHomePos = cast.focus ? cast.focus->Transform().GetWorldPos() - ISLAND_SUNK_OFFSET : islandHomePos;

        struct Step
        {
            std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> step;
            glm::vec3 homePos;
        };
        MoveBy(*cast.stairs, -ISLAND_SUNK_OFFSET);
        std::vector<Step> steps;
        for (const auto& step : cast.stairs->Transform().GetChildren())
            steps.push_back({ step, step->Transform().GetWorldPos() });

        bool isReturned = false;
        const auto finish = [&]
        {
            if (isReturned)
                return;
            isReturned = true;
            islandTransform.SetWorldPos(islandHomePos);
            islandTransform.SetWorldRot(islandHomeRot);
            ShowIsland(cast);
            for (const auto& [step, homePos] : steps)
                step->Transform().SetWorldPos(homePos);
            RebuildColliders(*cast.island);
            RebuildColliders(*cast.stairs);
            if (onReturned)
                onReturned();
        };

        const auto placeIsland = [&](const float rise, const float tiltDegrees)
        {
            // 手前へ傾いたまま上がってきて、揺れながら水平に戻る
            const glm::quat tilt = glm::angleAxis(glm::radians(tiltDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::vec3 sink(0.0f, -ISLAND_RISE_DEPTH * (1.0f - rise), 0.0f);
            islandTransform.SetWorldPos(pivotHomePos + tilt * (islandHomePos - pivotHomePos) + sink);
            islandTransform.SetWorldRot(tilt * islandHomeRot);
        };

        placeIsland(0.0f, ISLAND_RISE_TILT_DEGREES);
        cast.island->SetEnable(true);

        MovieScope scope(cast);
        scope.Begin();
        SkipInput skip;

        const float stairsStart_secs = ISLAND_RISE_SECS + STAIRS_DELAY_SECS;
        const float stairsEnd_secs   = stairsStart_secs + STAIRS_INTERVAL_SECS * static_cast<float>(steps.size()) + STAIRS_STEP_SECS;
        const float end_secs         = stairsEnd_secs + ISLAND_HOLD_SECS;

        float elapsed_secs = 0.0f;
        std::size_t shownSteps = 0;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            if (*isCanceled)
            {
                finish();
                co_return;
            }
            elapsed_secs += Time::DeltaTime();
            if (skip.IsSkipped() && elapsed_secs >= STONE_SKIP_GRACE_SECS)
                break;

            const float riseRate = Rate(elapsed_secs, ISLAND_RISE_SECS);
            const float wobble   = std::cos(riseRate * 3.14159265f * 3.0f) * (1.0f - EaseOutCubic(riseRate));
            placeIsland(EaseOutCubic(riseRate), ISLAND_RISE_TILT_DEGREES * wobble);

            for (std::size_t i = 0; i < steps.size(); ++i)
            {
                const float stepStart_secs = stairsStart_secs + STAIRS_INTERVAL_SECS * static_cast<float>(i);
                if (elapsed_secs < stepStart_secs)
                    break;

                if (i >= shownSteps)
                {
                    steps[i].step->SetEnable(true);
                    shownSteps = i + 1;
                }
                // 下から跳ね上がって、少し行き過ぎてから収まる
                const float t = Rate(elapsed_secs - stepStart_secs, STAIRS_STEP_SECS);
                const glm::vec3 drop(0.0f, -STAIRS_STEP_DROP * (1.0f - EaseOutBack(t)), 0.0f);
                steps[i].step->Transform().SetWorldPos(steps[i].homePos + drop);
            }
        }

        finish();
        scope.End();
    }
}
