#include "SoundFile.h"
#include "DxLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

NanamiEngine::Module::Asset::SoundFile::SoundFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
}

NanamiEngine::Module::Asset::SoundFile::~SoundFile()
{
    if (dxLibHandle_ == -1)
        return;

    DeleteSoundMem(dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::OnEnableAsset()
{
    dxLibHandle_ = LoadSoundMem(contentPath_.c_str());
    ChangeVolumeSoundMem(volume_, dxLibHandle_);
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
    if (dxLibHandle_ != -1)
        ChangeVolumeSoundMem(volume, dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::SetNextPlayVolume(const int volume) const
{
    if (dxLibHandle_ != -1)
        ChangeNextPlayVolumeSoundMem(volume, dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::Set3DPosition(const glm::vec3& position) const
{
    if (dxLibHandle_ != -1)
        Set3DPositionSoundMem({ position.x, position.y, position.z }, dxLibHandle_);
}

void NanamiEngine::Module::Asset::SoundFile::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);

    if (ImGui::SliderInt("volume_", &volume_, 0, 255))
    {
        ChangeVolumeSoundMem(volume_, dxLibHandle_);
    }

    ImGui::Text("dxLibId: %d", dxLibHandle_);
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::SoundFile, NanamiEngine::Module::Asset::AssetBase);
REGISTER_ASSET(SoundFile, ".mp3")
#pragma endregion
