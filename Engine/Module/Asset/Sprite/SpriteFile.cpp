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

    std::string SpriteFile::GetContentPath() const
    {
        return contentPath_;
    }
}
