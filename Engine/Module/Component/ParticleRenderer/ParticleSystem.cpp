#include "ParticleSystem.h"
#include "EffekseerForDXLib.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"

void Component::ParticleSystem::Play()
{
    TryDeleteResource();
    
    if (!IsEnable())
        return;

    firstUpdate_ = true;
    playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
    TryUpdateRenderPos();
    TryUpdateRenderRot();
    TryUpdateRenderScale();
}

void Component::ParticleSystem::OnUpdate()
{
    if (!IsEnable())
        return;
    
    playingDuring_secs_ += Time::DeltaTime();
}

void Component::ParticleSystem::InitRenderer()
{
    if (!particleFile_ || !IsEnable())
        return;

    resourceEffectHandle_ = particleFile_->LoadDxLibHandle();
    playingEffectHandle_  = PlayEffekseer3DEffect(resourceEffectHandle_);
}

void Component::ParticleSystem::OnRender()
{
    if (!IsEnable())
        return;

    if (isRoop_ && playingDuring_secs_ >= playingDuration_secs_)
    {
        firstUpdate_ = true;
        TryDeleteResource();
        playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
        playingDuring_secs_  = 0;
    }
    
    TryUpdateRenderPos();
    TryUpdateRenderRot();
    TryUpdateRenderScale();

    firstUpdate_ = false;
}

void Component::ParticleSystem::TryUpdateRenderPos()
{
    const auto pos = Transform().GetWorldPos();

    if (firstUpdate_ || pos != prevPos_)
    {
        SetPosPlayingEffekseer3DEffect(playingEffectHandle_, pos.x, pos.y, pos.z);
        prevPos_ = pos;
    }
}

void Component::ParticleSystem::TryUpdateRenderRot()
{
    const auto rot = Transform().GetWorldRot();

    if (firstUpdate_ || rot != prevRot_)
    {
        const auto eulerAngle = glm::eulerAngles(rot);
        SetRotationPlayingEffekseer3DEffect(playingEffectHandle_, eulerAngle.x, eulerAngle.y, eulerAngle.z);
        prevRot_ = rot;
    }
}

void Component::ParticleSystem::TryUpdateRenderScale()
{
    const auto scale = Transform().GetWorldScale();

    if (firstUpdate_ || scale != prevScale_)
    {
        SetScalePlayingEffekseer3DEffect(playingEffectHandle_, scale.x, scale.y, scale.z);
        prevScale_ = scale;
    }
}

void Component::ParticleSystem::OnDestroy()
{
    TryDeleteResource();
}

void Component::ParticleSystem::TryDeleteResource()
{
    if (playingEffectHandle_ != -1)
    {
        DeleteEffekseerEffect(playingEffectHandle_);
        playingEffectHandle_ = -1;
    }
}

void Component::ParticleSystem::OnDrawGui()
{
    if (ImGui::Button("Change Enable"))
    {
        SetEnable(!IsEnable());
    }
    
    ImGuiHelper::OnDrawInputField("particleFile_", particleFile_);
    ImGuiHelper::OnDrawInputField("resourceEffectHandle_", resourceEffectHandle_);
    ImGuiHelper::OnDrawInputField("playingEffectHandle_", playingEffectHandle_);
    ImGuiHelper::OnDrawInputField("playingDuration_secs_", playingDuration_secs_);
    ImGuiHelper::OnDrawInputField("playingDuring_secs_", playingDuring_secs_);
    ImGuiHelper::OnDrawInputField("isRoop_", isRoop_);

    if (ImGui::Button("Load EffectResource"))
    {
        resourceEffectHandle_ = particleFile_->LoadDxLibHandle();
    }
    if (ImGui::Button("Play Effect"))
    {
        Play();
    }
}
