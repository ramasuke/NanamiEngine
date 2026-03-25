#include "SoundFile.h"
#include "DxLib.h"

NanamiEngine::Module::Asset::SoundFile::SoundFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
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
