#include "MovieFile.h"

#include "DxLib.h"

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
        return LoadGraph(contentPath_.c_str());
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
