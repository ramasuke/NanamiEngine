#include "ParticleFile.h"

#include "EffekseerForDXLib.h"

namespace NanamiEngine::Module::Asset
{
    ParticleFile::ParticleFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
        
    }

    void ParticleFile::OnEnableAsset()
    {
        
    }

    const Guid& ParticleFile::GetGuid        () const { return guid_; }
    int         ParticleFile::LoadDxLibHandle() const
    {
        if (GetUseASyncLoadFlag())
            SetUseASyncLoadFlag(false);
        
        return LoadEffekseerEffect(contentPath_.c_str());
    }
    std::string ParticleFile::GetContentPath () const { return contentPath_; }
}
