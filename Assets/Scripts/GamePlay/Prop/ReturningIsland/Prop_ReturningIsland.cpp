#include "Prop_ReturningIsland.h"

#include <vector>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../FloatingStone/Prop_StoryMovieParts.h"

namespace GamePlay::Prop
{
    namespace
    {
        // 戻る前の島と階段は、雲のずっと下に退避しておく(隠してもコライダーは当たり続けるため)
        const glm::vec3 SUNK_OFFSET(0.0f, -3000.0f, 0.0f);
    }

    void ReturningIsland::Sink()
    {
        for (const auto& target : { Entity().lock(), stairs_.get() })
        {
            if (!target)
                continue;

            target->SetEnable(false);
            StoryMovie::MoveBy(*target, SUNK_OFFSET);
            StoryMovie::RebuildColliders(*target);
        }
    }

    void ReturningIsland::Show()
    {
        for (const auto& target : { Entity().lock(), stairs_.get() })
        {
            if (target)
                target->SetEnable(true);
        }
    }

    Coroutine::Task<void> ReturningIsland::PlayReturnAsync(
        std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar, std::function<bool()> canStart, std::function<void()> onReturned)
    {
        // NOTE: シーンを抜けて破棄されても、このコルーチンが終わるまでは this と島を生かしておく
        const auto self   = Components().Catch<ReturningIsland>().lock();
        const auto island = Entity().lock();
        const auto stairs = stairs_.get();
        const auto focus  = focus_.get();
        const IslandReturnShot shot = shot_;
        if (!stairs)
            co_return;

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

        // NOTE: Sink で下ろしてあるので、シーン上の位置(戻った位置)はその分だけ上
        auto& islandTransform = island->Transform();
        const glm::vec3 islandHomePos = islandTransform.GetWorldPos() - SUNK_OFFSET;
        const glm::quat islandHomeRot = islandTransform.GetWorldRot();
        // 傾きは島の見える所(カメラが見る所)を中心にかける。島の原点はモデルの外にある
        const glm::vec3 pivotHomePos = focus ? focus->Transform().GetWorldPos() - SUNK_OFFSET : islandHomePos;

        struct Step
        {
            std::shared_ptr<GameObject::IGameObject> step;
            glm::vec3 homePos;
        };
        StoryMovie::MoveBy(*stairs, -SUNK_OFFSET);
        std::vector<Step> steps;
        for (const auto& step : stairs->Transform().GetChildren())
            steps.push_back({ step, step->Transform().GetWorldPos() });

        bool isReturned = false;
        const auto finish = [&]
        {
            if (isReturned)
                return;
            isReturned = true;
            islandTransform.SetWorldPos(islandHomePos);
            islandTransform.SetWorldRot(islandHomeRot);
            Show();
            for (const auto& [step, homePos] : steps)
                step->Transform().SetWorldPos(homePos);
            StoryMovie::RebuildColliders(*island);
            StoryMovie::RebuildColliders(*stairs);
            if (onReturned)
                onReturned();
        };

        const auto placeIsland = [&](const float rise, const float tiltDegrees)
        {
            // 手前へ傾いたまま上がってきて、揺れながら水平に戻る
            const glm::quat tilt = glm::angleAxis(glm::radians(tiltDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::vec3 sink(0.0f, -shot.riseDepth * (1.0f - rise), 0.0f);
            islandTransform.SetWorldPos(pivotHomePos + tilt * (islandHomePos - pivotHomePos) + sink);
            islandTransform.SetWorldRot(tilt * islandHomeRot);
        };

        placeIsland(0.0f, shot.riseTiltDegrees);
        island->SetEnable(true);

        StoryMovie::CameraScope scope(playerAvatar, camera_.get(), focus, glm::vec3(0.0f));
        scope.Begin();
        StoryMovie::SkipInput skip;

        const float stairsStart_secs = shot.rise_secs + shot.stairsDelay_secs;
        const float stairsEnd_secs   = stairsStart_secs + shot.stairsInterval_secs * static_cast<float>(steps.size()) + shot.stairsStep_secs;
        const float end_secs         = stairsEnd_secs + shot.hold_secs;

        float elapsed_secs = 0.0f;
        std::size_t shownSteps = 0;
        while (elapsed_secs < end_secs)
        {
            co_await Coroutine::WaitYield();
            if (IsCanceled())
            {
                finish();
                co_return;
            }
            elapsed_secs += Time::DeltaTime();
            if (skip.IsSkipped() && elapsed_secs >= shot.skipGrace_secs)
                break;

            const float riseRate = StoryMovie::Rate(elapsed_secs, shot.rise_secs);
            const float wobble   = std::cos(riseRate * 3.14159265f * 3.0f) * (1.0f - StoryMovie::EaseOutCubic(riseRate));
            placeIsland(StoryMovie::EaseOutCubic(riseRate), shot.riseTiltDegrees * wobble);

            for (std::size_t i = 0; i < steps.size(); ++i)
            {
                const float stepStart_secs = stairsStart_secs + shot.stairsInterval_secs * static_cast<float>(i);
                if (elapsed_secs < stepStart_secs)
                    break;

                if (i >= shownSteps)
                {
                    steps[i].step->SetEnable(true);
                    shownSteps = i + 1;
                }
                // 下から跳ね上がって、少し行き過ぎてから収まる
                const float t = StoryMovie::Rate(elapsed_secs - stepStart_secs, shot.stairsStep_secs);
                const glm::vec3 drop(0.0f, -shot.stairsStepDrop * (1.0f - StoryMovie::EaseOutBack(t)), 0.0f);
                steps[i].step->Transform().SetWorldPos(steps[i].homePos + drop);
            }
        }

        finish();
        scope.End();
    }

    void ReturningIsland::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stairs_", stairs_);
        ImGuiHelper::OnDrawInputField("camera_", camera_);
        ImGuiHelper::OnDrawInputField("focus_",  focus_);
        if (ImGui::TreeNode("shot_"))
        {
            shot_.OnDrawGui();
            ImGui::TreePop();
        }
    }

    void IslandReturnShot::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("skipGrace_secs",      skipGrace_secs);
        ImGuiHelper::OnDrawInputField("delay_secs",          delay_secs);
        ImGuiHelper::OnDrawInputField("rise_secs",           rise_secs);
        ImGuiHelper::OnDrawInputField("riseDepth",           riseDepth);
        ImGuiHelper::OnDrawInputField("riseTiltDegrees",     riseTiltDegrees);
        ImGuiHelper::OnDrawInputField("stairsDelay_secs",    stairsDelay_secs);
        ImGuiHelper::OnDrawInputField("stairsStep_secs",     stairsStep_secs);
        ImGuiHelper::OnDrawInputField("stairsInterval_secs", stairsInterval_secs);
        ImGuiHelper::OnDrawInputField("stairsStepDrop",      stairsStepDrop);
        ImGuiHelper::OnDrawInputField("hold_secs",           hold_secs);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ReturningIsland);
#pragma endregion
