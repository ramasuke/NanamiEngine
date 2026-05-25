#include "SpriteFile.h"
#include "DxLib.h"

namespace NanamiEngine::Module::Asset
{
    SpriteFile::SpriteFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    void SpriteFile::OnEnableAsset()
    {
        dxLibId_ = LoadGraph();
    }

    int SpriteFile::LoadGraph() const
    {
        return DxLib::LoadGraph(contentPath_.c_str());
    }

    void SpriteFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
        ImGui::Text(("dxLibId: " + std::to_string(dxLibId_)).c_str());
    }

    std::string SpriteFile::GetContentPath() const
    {
        return contentPath_;
    }
}
