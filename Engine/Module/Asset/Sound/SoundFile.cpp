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
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SoundFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::SoundFile);
REGISTER_ASSET(SoundFile, ".mp3")
#pragma endregion
