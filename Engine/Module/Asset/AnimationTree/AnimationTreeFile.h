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
        [[nodiscard]] std::shared_ptr<AnimationTree::AnimationTree> OnLoadCopyContent() const { return std::make_shared<AnimationTree::AnimationTree>(contentPath_); }
        [[nodiscard]] std::string GetContentPath() const override;
        
    private:
        void OnDoubleClick () override;
        void OnSaveCallback() override;

        [[serialize(0)]] std::string contentPath_;
        [[serialize(0)]] Guid guid_;
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
}

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
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::AnimationTreeFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::AnimationTreeFile);
#pragma endregion

REGISTER_ASSET(AnimationTreeFile, ANIMATION_TREE_FILE_EXTENSION_LABEL)
REGISTER_CREATABLE_ASSET_EXTENSION("AnimationTree", ANIMATION_TREE_FILE_EXTENSION_LABEL)