#include "Enemy_Behaviour_Action_FallIsland.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    struct FallMotion
    {
        glm::vec3 pivot;
        glm::vec3 axis;
        float tiltAngleDeg;
        float tiltSecs;
        float fallAngleDeg;
        float fallDistance;
        float fallSecs;
        float tiltSinkDistance;
    };

    Coroutine::Task<void> FallAsync(const std::weak_ptr<GameObject::IGameObject> target, const FallMotion motion)
    {
        glm::vec3 startPos{};
        glm::quat startRot{};
        {
            const auto object = target.lock();
            if (!object)
                co_return;

            startPos = object->Transform().GetWorldPos();
            startRot = object->Transform().GetWorldRot();
        }

        const auto apply = [&](const float angleDeg, const float drop)
        {
            const auto object = target.lock();
            if (!object)
                return false;

            const glm::quat tilt = glm::angleAxis(glm::radians(angleDeg), motion.axis);
            object->Transform().SetWorldPos(motion.pivot + tilt * (startPos - motion.pivot) - glm::vec3(0.0f, drop, 0.0f));
            object->Transform().SetWorldRot(tilt * startRot);
            return true;
        };

        // ぐらりと傾く。行き過ぎて少し揺れ戻る
        for (float t = 0.0f; t < motion.tiltSecs; t += Time::DeltaTime())
        {
            const float u      = t / motion.tiltSecs;
            const float ease   = 0.5f - 0.5f * glm::cos(glm::pi<float>() * u);
            const float wobble = 0.3f * glm::sin(3.0f * glm::pi<float>() * u) * (1.0f - u);
            if (!apply(motion.tiltAngleDeg * (ease + wobble), motion.tiltSinkDistance * u))
                co_return;

            co_await Coroutine::WaitYield();
        }

        // 傾いたまま加速して落ちていく
        for (float t = 0.0f; t < motion.fallSecs; t += Time::DeltaTime())
        {
            const float u = t / motion.fallSecs;
            if (!apply(motion.tiltAngleDeg + motion.fallAngleDeg * glm::pow(u, 1.5f), motion.tiltSinkDistance + motion.fallDistance * u * u))
                co_return;

            co_await Coroutine::WaitYield();
        }

        if (const auto object = target.lock())
            object->SetEnable(false);
    }
}

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::FallIsland::DoTick(const TickContext& context)
    {
        if (!target_)
            return TickStatus::Failure;

        const glm::vec3 axis = glm::length(tiltAxis_) > 0.0001f ? glm::normalize(tiltAxis_) : glm::vec3(0.0f, 0.0f, 1.0f);
        const FallMotion motion
        {
            pivot_, axis, tiltAngleDeg_, glm::max(tiltSecs_, 0.01f), fallAngleDeg_, fallDistance_, glm::max(fallSecs_, 0.01f), tiltSinkDistance_
        };

        // NOTE: 落下は同期しない。序章(シングルプレイ)の演出用
        Coroutine::StartCoroutine(FallAsync(target_.get(), motion));
        return TickStatus::Success;
    }

    void Action::FallIsland::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("target_", target_);
        ImGuiHelper::OnDrawInputField("pivot_", pivot_);
        ImGuiHelper::OnDrawInputField("tiltAxis_", tiltAxis_);
        ImGuiHelper::OnDrawInputField("tiltAngleDeg_", tiltAngleDeg_);
        ImGuiHelper::OnDrawInputField("tiltSecs_", tiltSecs_);
        ImGuiHelper::OnDrawInputField("fallAngleDeg_", fallAngleDeg_);
        ImGuiHelper::OnDrawInputField("fallDistance_", fallDistance_);
        ImGuiHelper::OnDrawInputField("fallSecs_", fallSecs_);
        ImGuiHelper::OnDrawInputField("tiltSinkDistance_", tiltSinkDistance_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::FallIsland, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
