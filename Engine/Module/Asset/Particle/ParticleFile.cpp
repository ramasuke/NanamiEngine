#include "ParticleFile.h"

#include "EffekseerForDXLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    ParticleFile::ParticleFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
        
    }

    void ParticleFile::OnEnableAsset()
    {
        
    }

    void ParticleFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
    }

    const Guid& ParticleFile::GetGuid        () const { return guid_; }
    int         ParticleFile::LoadDxLibHandle() const
    {
        const int useASyncLoad = GetUseASyncLoadFlag();
        SetUseASyncLoadFlag(FALSE);
        const int handle = LoadEffekseerEffect(contentPath_.c_str());
        SetUseASyncLoadFlag(useASyncLoad);
        return handle;
    }
    std::string ParticleFile::GetContentPath () const { return contentPath_; }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::ParticleFile, NanamiEngine::Module::Asset::AssetBase);
REGISTER_ASSET(ParticleFile, ".efkefc")
#pragma endregion
