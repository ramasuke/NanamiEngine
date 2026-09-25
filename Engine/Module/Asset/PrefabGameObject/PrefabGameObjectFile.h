#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../GameObject/PrefabGameObject/PrefabGameObject.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto PREFAB_FILE_EXTENSION_LABEL = ".prefab";
    
    class NANAMI_API PrefabGameObjectFile final : public AssetBase,
                                       public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit PrefabGameObjectFile(std::string contentPath = "");
        [[nodiscard]] const Guid& GetGuid() const override  { return guid_; }
        void OnEnableAsset() override;
        std::shared_ptr<GameObject::PrefabGameObject> Content() { return content_; }
        [[nodiscard]] std::string GetContentPath() const override;

    private:
        void OnDoubleClick() override;
        void OnSaveCallback() override;
        void CopiedInit() override;
        void OnRenamed(const std::string& newContentPath) override;
        
        std::shared_ptr<GameObject::PrefabGameObject> content_;
        std::string contentPath_;
        Guid guid_;
        
#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<AssetBase>(this));
    if (version == 0)archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
    archive(CEREAL_NVP(contentPath_));
    archive(CEREAL_NVP(guid_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<AssetBase>(this));
    if (version == 0)archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
    if (version >= 0) archive(CEREAL_NVP(contentPath_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
}
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::PrefabGameObjectFile, 1);
#pragma endregion
