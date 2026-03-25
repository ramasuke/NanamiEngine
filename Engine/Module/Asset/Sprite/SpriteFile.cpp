#include "SpriteFile.h"
#include "DxLib.h"

NanamiEngine::Module::Asset::SpriteFile::SpriteFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
}

void NanamiEngine::Module::Asset::SpriteFile::OnEnableAsset()
{
    dxLibId_ = LoadGraph(contentPath_.c_str());
}

std::string NanamiEngine::Module::Asset::SpriteFile::GetContentPath() const
{
    return contentPath_;
}
