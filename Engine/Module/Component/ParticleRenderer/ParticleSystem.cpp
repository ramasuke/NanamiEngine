#include "ParticleSystem.h"
#include "EffekseerForDXLib.h"
#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

void Component::ParticleSystem::Play()
{
    TryStopPlaying();
    
    if (!IsEnable())
        return;

    firstUpdate_ = true;
    playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
    TryUpdateRenderPos();
    TryUpdateRenderRot();
    TryUpdateRenderScale();
}

void Component::ParticleSystem::Stop()
{
    TryStopPlaying();
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

    TryReleaseEffectResource();
    resourceEffectHandle_ = particleFile_->LoadDxLibHandle();
    if (IsEnable() && playMode_ != Particle::PlayMode::Manual)
    {
        playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
    }
}

void Component::ParticleSystem::OnRender()
{
    if (!IsEnable())
        return;

    switch (playMode_)
    {
    case Particle::PlayMode::Loop:
        if (playingDuring_secs_ >= playingDuration_secs_)
        {
            firstUpdate_ = true;
            TryStopPlaying();
            playingEffectHandle_ = PlayEffekseer3DEffect(resourceEffectHandle_);
            playingDuring_secs_  = 0;
        }
        break;
    case Particle::PlayMode::Destroy:
        if (playingDuring_secs_ >= playingDuration_secs_)
        {
            Entity().lock()->OnDestroy();
        }
        break;
    case Particle::PlayMode::Manual:
        break;
    default:
        throw std::exception("unknown type Play Mode");
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
    TryStopPlaying();
    TryReleaseEffectResource();
}

void Component::ParticleSystem::TryStopPlaying()
{
    if (playingEffectHandle_ != -1)
    {
        // 再生ハンドルは StopEffekseer3DEffect で止める（DeleteEffekseerEffect はリソースハンドル専用）
        StopEffekseer3DEffect(playingEffectHandle_);
        playingEffectHandle_ = -1;
    }
}

void Component::ParticleSystem::TryReleaseEffectResource()
{
    if (resourceEffectHandle_ != -1)
    {
        // ParticleFile::LoadDxLibHandle はインスタンスごとに新しく読み込むので、読み込んだ Component 側で解放する
        DeleteEffekseerEffect(resourceEffectHandle_);
        resourceEffectHandle_ = -1;
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
    ImGuiHelper::OnDrawEnumField("playMode_", playMode_, Particle::SCENE_TYPES, Particle::ToString);

    if (ImGui::Button("Load EffectResource"))
    {
        TryStopPlaying();
        TryReleaseEffectResource();
        resourceEffectHandle_ = particleFile_->LoadDxLibHandle();
    }
    if (ImGui::Button("Play Effect"))
    {
        Play();
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::ParticleSystem);
#pragma endregion
