#include "ParticleSystem.h"
#include "EffekseerForDXLib.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"

void Component::ParticleSystem::Play()
{
    playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
}

void Component::ParticleSystem::OnUpdate()
{
    if (!IsEnable())
        return;
    
    playingDuring_secs_ += Time::DeltaTime();
}

void Component::ParticleSystem::InitRenderer()
{
    if (!particleFile_)
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
        playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
        playingDuring_secs_  = 0;
    }
    
    OnUpdateRenderPos  ();
    OnUpdateRenderRot  ();
    OnUpdateRenderScale();
}

void Component::ParticleSystem::OnUpdateRenderPos() const
{
    const auto position = Transform().GetWorldPos();
    SetPosPlayingEffekseer3DEffect(playingEffectHandle_, position.x, position.y, position.z);
}

void Component::ParticleSystem::OnUpdateRenderRot() const
{
    const auto quatRotation = Transform().GetWorldRot();
    const auto eulerRotation = glm::eulerAngles(quatRotation);
    SetRotationPlayingEffekseer3DEffect(playingEffectHandle_, eulerRotation.x, eulerRotation.y, eulerRotation.z);
}

void Component::ParticleSystem::OnUpdateRenderScale() const
{
    const auto scale = Transform().GetWorldScale();
    SetScalePlayingEffekseer3DEffect(playingEffectHandle_, scale.x, scale.y, scale.z);
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
        playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
        OnUpdateRenderPos  ();
        OnUpdateRenderRot  ();
        OnUpdateRenderScale();
    }
}