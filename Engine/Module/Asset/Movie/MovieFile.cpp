#include "MovieFile.h"

#include "DxLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    MovieFile::MovieFile(const std::string& contentPath)
        : contentPath_(contentPath)
    {
        
    }

    const Guid& MovieFile::GetGuid() const
    {
        return guid_;
    }

    int MovieFile::LoadDxLibHandle() const
    {
        return OpenMovieToGraph(contentPath_.c_str());
    }

    std::string MovieFile::GetContentPath() const
    {
        return contentPath_;
    }

    void MovieFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::MovieFile, NanamiEngine::Module::Asset::AssetBase);
REGISTER_ASSET(MovieFile, ".mp4")
#pragma endregion
