#include "GamePlay_Enemy_BodyPartWeakPoint.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"

namespace GamePlay::Npc::Enemy
{
    glm::vec3 BodyPartWeakPoint::LockOnPosition()
    {
        glm::vec3 sum(0.0f);
        int count = 0;
        for (const auto& child : Transform().GetAllChildren())
        {
            const auto collider = child->Components().Catch<Physics::ICollider>().lock();
            if (!collider)
                continue;

            sum += collider->CenterOfMassPosition().value_or(child->Transform().GetWorldPos());
            ++count;
        }
        return count > 0 ? sum / static_cast<float>(count) : Transform().GetWorldPos();
    }

    bool BodyPartWeakPoint::AccumulateDamage(const int damageValue)
    {
        accumulatedDamage_ += damageValue;

        if (isBroken_ || accumulatedDamage_ < durability_)
            return false;

        isBroken_ = true;
        return true;
    }

    void BodyPartWeakPoint::OpenWeakWindow(const float duration_secs)
    {
        const bool wasClosed = !IsWeakWindowOpen();
        weakWindowRemaining_secs_ = duration_secs;

        if (!wasClosed)
            return;

        hintRetriggerDuring_secs_ = 0.0f;
        SetHintPlaying(true);
    }

    std::shared_ptr<BodyPartWeakPoint> BodyPartWeakPoint::FindFrom(
        const std::shared_ptr<GameObject::IGameObject>& hitPart,
        const GameObject::IGameObject&                  stopAt)
    {
        auto current = hitPart;
        while (current && current.get() != &stopAt)
        {
            if (const auto weakPoint = current->Components().Catch<BodyPartWeakPoint>().lock())
                return weakPoint;

            current = current->Transform().GetParent();
        }
        return nullptr;
    }

    void BodyPartWeakPoint::OnUpdate()
    {
        if (!IsWeakWindowOpen())
            return;

        const float delta = Time::DeltaTime();
        weakWindowRemaining_secs_ -= delta;

        if (!IsWeakWindowOpen())
        {
            weakWindowRemaining_secs_ = 0.0f;
            SetHintPlaying(false);
            return;
        }

        hintRetriggerDuring_secs_ += delta;
        if (hintRetriggerDuring_secs_ < hintRetrigger_secs_)
            return;

        hintRetriggerDuring_secs_ = 0.0f;
        SetHintPlaying(true);
    }

    void BodyPartWeakPoint::SetHintPlaying(const bool isPlaying)
    {
        if (!weakWindowHint_)
            return;

        if (isPlaying)
            weakWindowHint_->Play();
        else
            weakWindowHint_->Stop();
    }

    void BodyPartWeakPoint::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("durability_", durability_);
        ImGuiHelper::OnDrawInputField("weakWindowHint_", weakWindowHint_);
        ImGuiHelper::OnDrawInputField("hintRetrigger_secs_", hintRetrigger_secs_);
        ImGuiHelper::OnDrawInputField("isStunOnBreak_", isStunOnBreak_);

        ImGui::Text("Accumulated: %d", accumulatedDamage_);
        ImGui::Text("Broken: %s", isBroken_ ? "true" : "false");
        ImGui::Text("WeakWindow: %.2f", weakWindowRemaining_secs_);
    }
}
