#include "GamePlay_ChargeBreakPillar.h"

#include <algorithm>
#include <cmath>

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../ChargeStuckObstacle/GamePlay_ChargeStuckObstacle.h"
#include "../../Sound/SoundPlayer.h"

namespace GamePlay::Prop
{
    std::shared_ptr<ChargeBreakPillar> ChargeBreakPillar::FindFrom(GameObject::IGameObject& hitObject)
    {
        if (auto pillar = hitObject.Components().Catch<ChargeBreakPillar>().lock())
            return pillar;

        for (auto current = hitObject.Transform().GetParent(); current; current = current->Transform().GetParent())
        {
            if (auto pillar = current->Components().Catch<ChargeBreakPillar>().lock())
                return pillar;
        }
        return nullptr;
    }

    std::shared_ptr<ChargeBreakPillar> ChargeBreakPillar::FindNear(const glm::vec3& position)
    {
        std::shared_ptr<ChargeBreakPillar> nearest;
        float nearestSq = 0.0f;
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [&nearest, &nearestSq, &position](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                const auto pillar = gameObject->Components().Catch<ChargeBreakPillar>().lock();
                if (!pillar)
                    return;

                const glm::vec3 delta  = pillar->Transform().GetWorldPos() - position;
                const float     distSq = glm::dot(delta, delta);
                if (nearest && distSq >= nearestSq)
                    return;

                nearest   = pillar;
                nearestSq = distSq;
            });
        return nearest;
    }

    std::shared_ptr<ChargeBreakPillar> ChargeBreakPillar::FindIntroTarget(const glm::vec3& position)
    {
        std::shared_ptr<ChargeBreakPillar> nearest;
        float nearestSq = 0.0f;
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [&nearest, &nearestSq, &position](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                const auto pillar = gameObject->Components().Catch<ChargeBreakPillar>().lock();
                if (!pillar || !pillar->isIntroTarget_ || pillar->isCollapsed_)
                    return;

                const glm::vec3 delta  = pillar->Transform().GetWorldPos() - position;
                const float     distSq = glm::dot(delta, delta);
                if (nearest && distSq >= nearestSq)
                    return;

                nearest   = pillar;
                nearestSq = distSq;
            });
        return nearest;
    }

    void ChargeBreakPillar::TrembleAll(const glm::vec3& center, const float radius)
    {
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [&center, radius](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                const auto pillar = gameObject->Components().Catch<ChargeBreakPillar>().lock();
                if (!pillar)
                    return;

                glm::vec3 delta = pillar->Transform().GetWorldPos() - center;
                delta.y = 0.0f;
                if (glm::dot(delta, delta) <= radius * radius)
                    pillar->Tremble();
            });
    }

    bool ChargeBreakPillar::Collapse(const glm::vec3& fallDirection)
    {
        if (isCollapsed_)
            return false;
        isCollapsed_ = true;
        isTrembling_ = false;

        if (const auto obstacle = Components().Catch<ChargeStuckObstacle>().lock())
            obstacle->SetEnable(false);
        if (const auto standingCollider = standingCollider_.get())
            standingCollider->OnDestroy();

        glm::vec3 direction(fallDirection.x, 0.0f, fallDirection.z);
        direction = glm::dot(direction, direction) > 1e-6f ? glm::normalize(direction) : glm::vec3(0.0f, 0.0f, -1.0f);

        if (const auto top = top_.get())
        {
            // NOTE: 揺れている途中なら、揺れる前の向きから倒す
            if (!topStandingRot_)
                topStandingRot_ = top->Transform().GetLocalRot();

            // NOTE: 上向きを direction へ傾ける軸を、top の親の空間へ持っていく
            glm::quat parentRot(1.0f, 0.0f, 0.0f, 0.0f);
            if (const auto parent = top->Transform().GetParent())
                parentRot = parent->Transform().GetWorldRot();
            fallAxis_ = glm::normalize(glm::inverse(parentRot) * glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), direction));
        }

        const glm::vec3 position = DustPosition();
        if (breakParticle_)
            Scene::GameObject::Instantiate(breakParticle_.get(), position);
        if (breakSound_)
            Sound::SoundPlayer::PlaySe(*breakSound_.get(), position);
        return true;
    }

    void ChargeBreakPillar::Tremble()
    {
        if (isCollapsed_)
            return;

        const auto top = top_.get();
        if (!top)
            return;

        if (!topStandingRot_)
            topStandingRot_ = top->Transform().GetLocalRot();

        // NOTE: 柱ごとに揺れる向きを変える。位置から決めるので全ピアで同じになる
        const glm::vec3 position = Transform().GetWorldPos();
        const float     angle    = std::fmod(position.x * 0.37f + position.z * 0.61f, glm::two_pi<float>());
        trembleAxis_         = glm::vec3(std::cos(angle), 0.0f, std::sin(angle));
        trembleElapsed_secs_ = 0.0f;
        isTrembling_         = true;

        const glm::vec3 dustPosition = DustPosition();
        if (trembleParticle_)
            Scene::GameObject::Instantiate(trembleParticle_.get(), dustPosition);
        if (trembleSound_)
            Sound::SoundPlayer::PlaySe(*trembleSound_.get(), dustPosition);
    }

    void ChargeBreakPillar::OnUpdate()
    {
        if (isTrembling_)
            UpdateTremble();
        if (isCollapsed_ && !isLanded_)
            UpdateFall();
    }

    void ChargeBreakPillar::UpdateTremble()
    {
        const auto top = top_.get();
        if (!top || !topStandingRot_)
        {
            isTrembling_ = false;
            return;
        }

        trembleElapsed_secs_ += Time::DeltaTime();
        const float t = tremble_secs_ > 0.0f ? std::clamp(trembleElapsed_secs_ / tremble_secs_, 0.0f, 1.0f) : 1.0f;
        // NOTE: 小刻みに往復させながら収める
        const float angle = trembleAngle_deg_ * (1.0f - t) * std::sin(trembleElapsed_secs_ * 38.0f);
        top->Transform().SetLocalRot(glm::angleAxis(glm::radians(angle), trembleAxis_) * *topStandingRot_);

        if (t >= 1.0f)
            isTrembling_ = false;
    }

    void ChargeBreakPillar::UpdateFall()
    {
        const auto top = top_.get();
        if (!top || !topStandingRot_)
        {
            isLanded_ = true;
            return;
        }

        fallElapsed_secs_ += Time::DeltaTime();
        const float t = fallDuration_secs_ > 0.0f ? std::clamp(fallElapsed_secs_ / fallDuration_secs_, 0.0f, 1.0f) : 1.0f;
        // NOTE: 倒れ始めはゆっくり、地面に近づくほど速くなる
        const float eased = t * t * t;
        top->Transform().SetLocalRot(glm::angleAxis(glm::radians(fallAngle_deg_ * eased), fallAxis_) * *topStandingRot_);

        if (t < 1.0f)
            return;

        isLanded_ = true;
        const glm::vec3 landPosition = top->Transform().GetWorldPos();
        if (landParticle_)
            Scene::GameObject::Instantiate(landParticle_.get(), landPosition);
        if (landSound_)
            Sound::SoundPlayer::PlaySe(*landSound_.get(), landPosition);
    }

    glm::vec3 ChargeBreakPillar::DustPosition() const
    {
        return dustPoint_ ? dustPoint_->Transform().GetWorldPos() : Transform().GetWorldPos();
    }

    void ChargeBreakPillar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("top_", top_);
        ImGuiHelper::OnDrawInputField("standingCollider_", standingCollider_);
        ImGuiHelper::OnDrawInputField("dustPoint_", dustPoint_);
        ImGuiHelper::OnDrawInputField("breakParticle_", breakParticle_);
        ImGuiHelper::OnDrawInputField("landParticle_", landParticle_);
        ImGuiHelper::OnDrawInputField("breakSound_", breakSound_);
        ImGuiHelper::OnDrawInputField("landSound_", landSound_);
        ImGuiHelper::OnDrawInputField("collapseDamage_", collapseDamage_);
        ImGuiHelper::OnDrawInputField("fallAngle_deg_", fallAngle_deg_);
        ImGuiHelper::OnDrawInputField("fallDuration_secs_", fallDuration_secs_);
        ImGuiHelper::OnDrawInputField("isIntroTarget_", isIntroTarget_);
        ImGuiHelper::OnDrawInputField("trembleParticle_", trembleParticle_);
        ImGuiHelper::OnDrawInputField("trembleSound_", trembleSound_);
        ImGuiHelper::OnDrawInputField("trembleAngle_deg_", trembleAngle_deg_);
        ImGuiHelper::OnDrawInputField("tremble_secs_", tremble_secs_);
        ImGui::Text("collapsed: %s  landed: %s", isCollapsed_ ? "true" : "false", isLanded_ ? "true" : "false");

        // NOTE: 配置の確認用。カメラの奥へ倒す
        if (!isCollapsed_ && ImGui::Button("Collapse (preview)"))
            Collapse(glm::vec3(0.0f, 0.0f, -1.0f));
        ImGui::SameLine();
        if (!isCollapsed_ && ImGui::Button("Tremble (preview)"))
            Tremble();
    }
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ChargeBreakPillar);
