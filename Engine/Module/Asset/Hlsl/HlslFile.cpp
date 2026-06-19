#include "HlslFile.h"

namespace NanamiEngine::Module::Asset
{
    HlslFile::HlslFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    std::string HlslFile::GetContentPath() const
    {
        return contentPath_;
    }

    void HlslFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
    }
}
