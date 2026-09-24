#include "Prop_FloatingStone.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Prop_StoryMovieParts.h"

namespace GamePlay::Prop
{
    namespace
    {
        glm::vec3 StoneCenter(GameObject::IGameObject& stone, const float centerHeight)
        {
            const auto& transform = stone.Transform();
            return transform.GetWorldPos() + transform.GetWorldRot() * glm::vec3(0.0f, centerHeight * transform.GetWorldScale().y, 0.0f);
        }

        /** @brief 飛んでいる石の中心に重ねる光の尾 */
        class FlightTrail final
        {
        public:
            FlightTrail(std::shared_ptr<Asset::PrefabGameObjectFile> prefab, GameObject::IGameObject& stone, const float centerHeight)
                : prefab_(std::move(prefab)), stone_(stone), centerHeight_(centerHeight)
            {
            }
            ~FlightTrail() { Destroy(); }

            void Spawn()
            {
                if (prefab_)
                    trail_ = Scene::GameObject::Instantiate(prefab_, StoneCenter(stone_, centerHeight_));
            }

            void Move() const
            {
                if (const auto trail = trail_.lock())
                    trail->Transform().SetWorldPos(StoneCenter(stone_, centerHeight_));
            }

            void Destroy()
            {
                if (const auto trail = trail_.lock())
                    trail->OnDestroy();
                trail_.reset();
            }

        private:
            std::shared_ptr<Asset::PrefabGameObjectFile> prefab_;
            GameObject::IGameObject& stone_;
            float centerHeight_;
            std::weak_ptr<GameObject::IGameObject> trail_;
        };
    }

    void FloatingStone::SetVisible(const bool isVisible)
    {
        const auto stone = Entity().lock();
        if (!stone)
            return;

        // NOTE: GameObject を無効にしても Effekseer の再生は残るので、パーティクルは先に止める
        if (!isVisible)
            StoryMovie::SetChildParticlesPlaying(*stone, false);
        stone->SetEnable(isVisible);
        if (isVisible)
            StoryMovie::SetChildParticlesPlaying(*stone, true);
    }

    Coroutine::Task<void> FloatingStone::PlayDepartAsync(std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar)
    {
        // NOTE: シーンを抜けて破棄されても、このコルーチンが終わるまでは this と石を生かしておく
        const auto self  = Components().Catch<FloatingStone>().lock();
        const auto stone = Entity().lock();
        const DepartShot shot = departShot_;

        float wait_secs = 0.0f;
        while (wait_secs < shot.delay_secs)
        {
            co_await Coroutine::WaitYield();
            if (IsCanceled())
                co_return;
            wait_secs += Time::DeltaTime();
        }

        auto& transform = stone->Transform();
        const glm::vec3 basePos = transform.GetWorldPos();
        const glm::quat baseRot = transform.GetWorldRot();
        const glm::vec3 risenPos = basePos + glm::vec3(0.0f, shot.riseHeight, 0.0f);

        StoryMovie::CameraScope scope(playerAvatar, camera_.get(), stone, StoneCenter(*stone, shot.stoneCenterHeight) - basePos);
        scope.Begin();
        FlightTrail trail(flightParticle_.get(), *stone, shot.stoneCenterHeight);
        StoryMovie::SkipInput skip;

        const float riseStart_secs = shot.shake_secs;
        const float flyStart_secs  = riseStart_secs + shot.rise_secs;
        const float holdStart_secs = flyStart_secs + shot.fly_secs;
        const float end_secs       = holdStart_secs + shot.hold_secs;

        float elapsed_secs = 0.0f;
        bool isLifted = false;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            if (IsCanceled())
                co_return;
            elapsed_secs += Time::DeltaTime();
            if (skip.IsSkipped() && elapsed_secs >= shot.skipGrace_secs)
                break;

            if (elapsed_secs < riseStart_secs)
            {
                // 力が溢れて、だんだん強く震える
                const float width = shot.shakeWidth * StoryMovie::Rate(elapsed_secs, shot.shake_secs);
                const glm::vec3 shake(std::sin(elapsed_secs * 53.0f), 0.0f, std::cos(elapsed_secs * 41.0f));
                transform.SetWorldPos(basePos + shake * width);
                continue;
            }

            if (!isLifted)
            {
                isLifted = true;
                StoryMovie::SetChildParticlesPlaying(*stone, false);
                if (burstParticle_)
                    Scene::GameObject::Instantiate(burstParticle_.get(), basePos);
                trail.Spawn();
            }

            if (elapsed_secs < flyStart_secs)
            {
                const float t = StoryMovie::Rate(elapsed_secs - riseStart_secs, shot.rise_secs);
                transform.SetWorldPos(glm::mix(basePos, risenPos, StoryMovie::EaseOutCubic(t)));
                transform.SetWorldRot(StoryMovie::Yaw(shot.riseTurnDegrees * StoryMovie::EaseInOutSine(t)) * baseRot);
            }
            else
            {
                const float t = StoryMovie::Rate(elapsed_secs - flyStart_secs, shot.fly_secs);
                transform.SetWorldPos(risenPos + shot.flyOffset * StoryMovie::EaseInCubic(t));
                transform.SetWorldRot(StoryMovie::Yaw(shot.riseTurnDegrees + shot.flyTurnDegrees * StoryMovie::EaseInCubic(t)) * baseRot);
            }
            trail.Move();
        }

        // 飛び去った石は隠し、置き場所だけ元へ戻しておく
        SetVisible(false);
        transform.SetWorldPos(basePos);
        transform.SetWorldRot(baseRot);
        trail.Destroy();
        scope.End();
    }

    Coroutine::Task<void> FloatingStone::PlayReturnAsync(
        std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar, std::function<bool()> canStart, std::function<void()> onDocked)
    {
        const auto self  = Components().Catch<FloatingStone>().lock();
        const auto stone = Entity().lock();
        const ReturnShot shot = returnShot_;

        // シーン切り替え直後は DeltaTime が 0 で、コルーチンごと凍る
        co_await Coroutine::WaitUntil([] { return Time::DeltaTime() > 0.0f; });
        if (IsCanceled())
            co_return;
        while (canStart && !canStart())
        {
            co_await Coroutine::WaitYield();
            if (IsCanceled())
                co_return;
        }
        float wait_secs = 0.0f;
        while (wait_secs < shot.delay_secs)
        {
            co_await Coroutine::WaitYield();
            if (IsCanceled())
                co_return;
            wait_secs += Time::DeltaTime();
        }

        auto& transform = stone->Transform();
        const glm::vec3 dockPos     = transform.GetWorldPos();
        const glm::quat dockRot     = transform.GetWorldRot();
        const glm::vec3 startPos    = dockPos + shot.startOffset;
        const glm::vec3 approachPos = dockPos + shot.approachOffset;

        bool isDocked = false;
        const auto dock = [&]
        {
            if (isDocked)
                return;
            isDocked = true;
            transform.SetWorldPos(dockPos);
            transform.SetWorldRot(dockRot);
            StoryMovie::SetChildParticlesPlaying(*stone, true);
            if (onDocked)
                onDocked();
        };

        SetVisible(true);
        StoryMovie::SetChildParticlesPlaying(*stone, false);
        transform.SetWorldPos(startPos);

        StoryMovie::CameraScope scope(playerAvatar, camera_.get(), stone, StoneCenter(*stone, shot.stoneCenterHeight) - startPos);
        scope.Begin();
        FlightTrail trail(flightParticle_.get(), *stone, shot.stoneCenterHeight);
        trail.Spawn();
        StoryMovie::SkipInput skip;

        const float settleStart_secs = shot.fly_secs;
        const float holdStart_secs   = settleStart_secs + shot.settle_secs;
        const float end_secs         = holdStart_secs + shot.hold_secs;

        float elapsed_secs = 0.0f;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            if (IsCanceled())
            {
                dock();
                co_return;
            }
            elapsed_secs += Time::DeltaTime();
            if (skip.IsSkipped() && elapsed_secs >= shot.skipGrace_secs)
                break;

            if (elapsed_secs < settleStart_secs)
            {
                // 回りながら飛んできて、底の真下で勢いを落とす
                const float t = StoryMovie::EaseOutCubic(StoryMovie::Rate(elapsed_secs, shot.fly_secs));
                transform.SetWorldPos(glm::mix(startPos, approachPos, t));
                transform.SetWorldRot(StoryMovie::Yaw(shot.flyTurnDegrees * (1.0f - t)) * dockRot);
                trail.Move();
            }
            else if (elapsed_secs < holdStart_secs)
            {
                const float t = StoryMovie::EaseInOutSine(StoryMovie::Rate(elapsed_secs - settleStart_secs, shot.settle_secs));
                transform.SetWorldPos(glm::mix(approachPos, dockPos, t));
                transform.SetWorldRot(dockRot);
                trail.Move();
            }
            else if (!isDocked)
            {
                trail.Destroy();
                if (burstParticle_)
                    Scene::GameObject::Instantiate(burstParticle_.get(), StoneCenter(*stone, shot.stoneCenterHeight));
                dock();
            }
        }

        dock();
        trail.Destroy();
        scope.End();
    }

    void FloatingStone::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("camera_",         camera_);
        ImGuiHelper::OnDrawInputField("flightParticle_", flightParticle_);
        ImGuiHelper::OnDrawInputField("burstParticle_",  burstParticle_);
        if (ImGui::TreeNode("departShot_"))
        {
            departShot_.OnDrawGui();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("returnShot_"))
        {
            returnShot_.OnDrawGui();
            ImGui::TreePop();
        }
    }

    void DepartShot::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("skipGrace_secs",    skipGrace_secs);
        ImGuiHelper::OnDrawInputField("stoneCenterHeight", stoneCenterHeight);
        ImGuiHelper::OnDrawInputField("delay_secs",        delay_secs);
        ImGuiHelper::OnDrawInputField("shake_secs",        shake_secs);
        ImGuiHelper::OnDrawInputField("rise_secs",         rise_secs);
        ImGuiHelper::OnDrawInputField("fly_secs",          fly_secs);
        ImGuiHelper::OnDrawInputField("hold_secs",         hold_secs);
        ImGuiHelper::OnDrawInputField("shakeWidth",        shakeWidth);
        ImGuiHelper::OnDrawInputField("riseHeight",        riseHeight);
        ImGuiHelper::OnDrawInputField("riseTurnDegrees",   riseTurnDegrees);
        ImGuiHelper::OnDrawInputField("flyTurnDegrees",    flyTurnDegrees);
        ImGuiHelper::OnDrawInputField("flyOffset",         flyOffset);
    }

    void ReturnShot::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("skipGrace_secs",    skipGrace_secs);
        ImGuiHelper::OnDrawInputField("stoneCenterHeight", stoneCenterHeight);
        ImGuiHelper::OnDrawInputField("delay_secs",        delay_secs);
        ImGuiHelper::OnDrawInputField("fly_secs",          fly_secs);
        ImGuiHelper::OnDrawInputField("settle_secs",       settle_secs);
        ImGuiHelper::OnDrawInputField("hold_secs",         hold_secs);
        ImGuiHelper::OnDrawInputField("flyTurnDegrees",    flyTurnDegrees);
        ImGuiHelper::OnDrawInputField("startOffset",       startOffset);
        ImGuiHelper::OnDrawInputField("approachOffset",    approachOffset);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::FloatingStone);
#pragma endregion
