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
        bool isAsyncLoad = false;
        if (GetUseASyncLoadFlag())
        {
            isAsyncLoad = SetUseASyncLoadFlag(false);
        }

        const int handle = LoadEffekseerEffect(contentPath_.c_str());
        if (isAsyncLoad)
        {
            SetUseASyncLoadFlag(true);
        }
        return handle;
    }
    std::string ParticleFile::GetContentPath () const { return contentPath_; }
}
