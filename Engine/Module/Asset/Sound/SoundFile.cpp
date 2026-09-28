#include "SoundFile.h"
#include "DxLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

#include <algorithm>

NanamiEngine::Module::Asset::SoundFile::SoundFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
}

NanamiEngine::Module::Asset::SoundFile::~SoundFile()
{
    NanamiEngine::Module::Audio::UnregisterSound(this);
    if (dxLibHandle_ == -1)
        return;

    DeleteSoundMem(dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::OnEnableAsset()
{
    dxLibHandle_ = LoadSoundMem(contentPath_.c_str());

    // NOTE: Assets/Audio/BGM 配下を音楽、それ以外を効果音として音量設定を掛ける
    auto path = contentPath_;
    std::ranges::replace(path, '/', '\\');
    category_ = path.find("Audio\\BGM\\") != std::string::npos
        ? NanamiEngine::Module::Audio::AudioCategory::Bgm
        : NanamiEngine::Module::Audio::AudioCategory::Se;

    currentVolume_ = volume_;
    NanamiEngine::Module::Audio::RegisterSound(this);
    ReapplyVolume();
}

std::string NanamiEngine::Module::Asset::SoundFile::GetContentPath() const
{
    return contentPath_;
}

void NanamiEngine::Module::Asset::SoundFile::Play(const bool loop, const bool restart) const
{
    if (dxLibHandle_ == -1)
        return;
    PlaySoundMem(dxLibHandle_, loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK, restart ? TRUE : FALSE);
}

void NanamiEngine::Module::Asset::SoundFile::Stop() const
{
    if (dxLibHandle_ != -1)
        StopSoundMem(dxLibHandle_);
}

bool NanamiEngine::Module::Asset::SoundFile::IsPlaying() const
{
    return dxLibHandle_ != -1 && CheckSoundMem(dxLibHandle_) == 1;
}

void NanamiEngine::Module::Asset::SoundFile::SetVolume(const int volume) const
{
    currentVolume_ = volume;
    ReapplyVolume();
}

void NanamiEngine::Module::Asset::SoundFile::SetNextPlayVolume(const int volume) const
{
    if (dxLibHandle_ != -1)
        ChangeNextPlayVolumeSoundMem(NanamiEngine::Module::Audio::ScaleVolume(category_, volume), dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::Set3DPosition(const glm::vec3& position) const
{
    if (dxLibHandle_ != -1)
        Set3DPositionSoundMem({ position.x, position.y, position.z }, dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::ReapplyVolume() const
{
    if (dxLibHandle_ != -1)
        ChangeVolumeSoundMem(NanamiEngine::Module::Audio::ScaleVolume(category_, currentVolume_), dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);

    if (ImGui::SliderInt("volume_", &volume_, 0, 255))
    {
        currentVolume_ = volume_;
        ReapplyVolume();
    }

    ImGui::Text("dxLibId: %d", dxLibHandle_);
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::SoundFile, NanamiEngine::Module::Asset::AssetBase);
REGISTER_ASSET(SoundFile, ".mp3")
#pragma endregion
