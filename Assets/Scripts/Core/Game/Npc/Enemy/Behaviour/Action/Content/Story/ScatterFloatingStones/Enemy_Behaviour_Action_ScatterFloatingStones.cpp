#include "Enemy_Behaviour_Action_ScatterFloatingStones.h"

#include <vector>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../../GamePlay/Prop/FloatingStone/Prop_StoryMovieParts.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    namespace
    {
        namespace StoryMovie = GamePlay::Prop::StoryMovie;
    }

    Coroutine::Task<void> Action::ScatterFloatingStones::PlayScatterAsync(std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stonesRoot, const ScatterShot shot)
    {
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
            StoryMovie::SetChildParticlesPlaying(*stone, false);
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
                    StoryMovie::SetChildParticlesPlaying(*flight.stone, true);
            }

            for (const auto& flight : flights)
            {
                auto& transform = flight.stone->Transform();
                if (elapsed_secs < hoverStart_secs)
                {
                    // 地面を割ってせり上がり、回りながら減速する
                    const float t = StoryMovie::EaseOutCubic(StoryMovie::Rate(elapsed_secs, shot.rise_secs));
                    transform.SetWorldPos(glm::mix(flight.buriedPos, flight.risenPos, t));
                    transform.SetWorldRot(StoryMovie::Yaw(180.0f * t) * flight.baseRot);
                }
                else if (elapsed_secs < flyStart_secs)
                {
                    const float hover_secs = elapsed_secs - hoverStart_secs;
                    transform.SetWorldPos(flight.risenPos + glm::vec3(0.0f, std::sin(hover_secs * 6.0f) * 2.0f, 0.0f));
                    transform.SetWorldRot(StoryMovie::Yaw(180.0f + 40.0f * hover_secs) * flight.baseRot);
                }
                else
                {
                    // 水平は加速しながら、高さは一定の速さで上がるので、弧を描いて空へ抜ける
                    const float rate = StoryMovie::Rate(elapsed_secs - flyStart_secs, shot.fly_secs);
                    transform.SetWorldPos(flight.risenPos
                        + flight.direction * (shot.flyDistance * StoryMovie::EaseInCubic(rate))
                        + glm::vec3(0.0f, shot.flyRise * rate, 0.0f));
                    transform.SetWorldRot(StoryMovie::Yaw(180.0f + 40.0f * shot.hover_secs + 720.0f * rate) * flight.baseRot);
                }
            }
        }

        // NOTE: GameObject を無効にしても Effekseer の再生は残るので、パーティクルは先に止める
        for (const auto& flight : flights)
        {
            StoryMovie::SetChildParticlesPlaying(*flight.stone, false);
            flight.stone->SetEnable(false);
        }
    }

    TickStatus Action::ScatterFloatingStones::DoTick(const TickContext& context)
    {
        if (!stonesRoot_)
            return TickStatus::Failure;

        Coroutine::StartCoroutine(PlayScatterAsync(
            stonesRoot_.get(),
            ScatterShot{ riseHeight_, riseSeconds_, hoverSeconds_, flySeconds_, flyDistance_, flyRise_ }));
        return TickStatus::Success;
    }

    void Action::ScatterFloatingStones::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stonesRoot_", stonesRoot_);
        ImGuiHelper::OnDrawInputField("riseHeight_", riseHeight_);
        ImGuiHelper::OnDrawInputField("riseSeconds_", riseSeconds_);
        ImGuiHelper::OnDrawInputField("hoverSeconds_", hoverSeconds_);
        ImGuiHelper::OnDrawInputField("flySeconds_", flySeconds_);
        ImGuiHelper::OnDrawInputField("flyDistance_", flyDistance_);
        ImGuiHelper::OnDrawInputField("flyRise_", flyRise_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ScatterFloatingStones);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ScatterFloatingStones);
#pragma endregion
