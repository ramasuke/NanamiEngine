#pragma once
#include <string>

#include "../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"
#include "../Preload/Engine_Asset_IPreloadableAsset.h"

namespace NanamiEngine::Module::Asset
{
    class Mv1File final : public AssetBase,
                          public LifeCycleCallback::IEnablableAsset,
                          public IPreloadableAsset
    {
    public:
        explicit Mv1File(const std::string& contentPath = "");
        ~Mv1File() override;
        Mv1File(const Mv1File&)            = delete;
        Mv1File& operator=(const Mv1File&) = delete;
        [[nodiscard]] const Guid& GetGuid       () const override;
        /** @brief 元モデルが未読込ならここで読み終えてから複製を返す */
        [[nodiscard]] int         LoadDxLibHandle   () const;
        [[nodiscard]] std::string GetContentPath() const override;
        /** @brief 非同期ロードが完了して LoadDxLibHandle() が使える状態か。未読込なら読み込みを要求する */
        [[nodiscard]] bool        IsLoadCompleted() const;
        void RequestLoad() const override;
        void Unload() override;

    private:
        void OnEnableAsset() override;
        void OnDoubleClick() override;
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

        std::string contentPath_;
        Guid guid_;
        mutable int  dxLibHandle_     = -1;
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
        // version 0 はハンドル値を保存していた。デストラクタで解放するため、古い値はメンバに入れず読み捨てる
        int legacyDxLibHandle = -1;
        if (version == 0) archive(cereal::make_nvp("dxLibHandle_", legacyDxLibHandle));
    }
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::Mv1File, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::Mv1File);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::Mv1File);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::Mv1File);
#pragma endregion
REGISTER_ASSET(Mv1File, ".mv1")
