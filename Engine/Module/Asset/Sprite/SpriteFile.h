#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "../../Guid/Guid.h"
#include "../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"
#include "../Preload/Engine_Asset_IPreloadableAsset.h"

namespace NanamiEngine::Module::Asset
{
    class NANAMI_API SpriteFile final : public AssetBase,
                             public LifeCycleCallback::IEnablableAsset,
                             public IPreloadableAsset
    {
    public:
        explicit SpriteFile(std::string contentPath = "");
        ~SpriteFile() override;
        SpriteFile(const SpriteFile&)            = delete;
        SpriteFile& operator=(const SpriteFile&) = delete;
        void OnEnableAsset() override;
        [[nodiscard]] const Guid& GetGuid        () const override { return guid_;     }
        /** @brief 未読込ならここで読み終えてから返す */
        [[nodiscard]] int         GetDxLibHandle () const;
        [[nodiscard]] std::string GetContentPath () const override;
        void RequestLoad() const override;
        void Unload() override;

    private:
        [[nodiscard]] int         LoadGraph() const;
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

        std::string contentPath_;
        Guid guid_;
        mutable int  dxLibId_         = -1;
        mutable bool isLoadAttempted_ = false;
#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<AssetBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
    archive(CEREAL_NVP(contentPath_));  
    archive(CEREAL_NVP(guid_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<AssetBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
    if (version >= 0) archive(CEREAL_NVP(contentPath_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SpriteFile, 0);
#pragma endregion
