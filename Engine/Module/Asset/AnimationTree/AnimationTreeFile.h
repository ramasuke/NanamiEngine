#pragma once
#include "../../../Core/Object/IObject.h"
#include "../../AnimationTree/AnimationTree.h"
#include "../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../AssetBase.h"

constexpr auto ANIMATION_TREE_FILE_EXTENSION_LABEL = ".animTree";

namespace NanamiEngine::Module::Asset
{
    class AnimationTreeFile final : public AssetBase
    {
    public:
        explicit AnimationTreeFile(std::string contentPath = "");
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        /** AnimationTree を読み込んで返す。ファイルが壊れている場合はエラーを記録して nullptr を返す */
        [[nodiscard]] std::shared_ptr<AnimationTree::AnimationTree> OnLoadCopyContent() const;
        [[nodiscard]] std::string GetContentPath() const override;
        
    private:
        void OnDoubleClick () override;
        void OnSaveCallback() override;
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

        [[serialize(0)]] std::string contentPath_;
        [[serialize(0)]] Guid guid_;
#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<AssetBase>(this));
    archive(CEREAL_NVP(contentPath_));
    archive(CEREAL_NVP(guid_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<AssetBase>(this));
    if (version >= 0) archive(CEREAL_NVP(contentPath_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::AnimationTreeFile, 0);
#pragma endregion
