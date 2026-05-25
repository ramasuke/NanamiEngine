#pragma once
#include "../SpriteFile.h"
#include "../../../../Core/Object/Field/Field.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto SPRITE_ANIMATION_FILE_EXTENSION_LABEL = ".spriteAnimation";
    
    class SpriteAnimationFile final : public AssetBase,
                                      public LifeCycleCallback::IEnablableAsset
    {
    public:
        enum class AnimationSourceType : int
        {
            Individual, //1枚ずつ
            SpriteSheet //1枚から分割
        };

        explicit SpriteAnimationFile(std::string contentPath = "");
        [[nodiscard]] const std::vector<int>& GetSpritesHandle() const { return spritesDxlibHandle_; }
        [[nodiscard]] std::string GetContentPath() const override { return contentPath_; }

    private:
        void OnEnableAsset() override;
        void OnSaveCallback() override;
        void LoadSprite();
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; } 

        
        [[serialize(0)]] std::vector<int> spritesDxlibHandle_;
        [[serialize(0)]] AnimationSourceType sourceType_ = AnimationSourceType::Individual;
        [[serialize(0)]] std::vector<FIELD(SpriteFile)> sprites_;
        
        /** SpriteSheet */
        [[serialize(0)]] FIELD(SpriteFile) sprite_;
        [[serialize(0)]] int splitCount_  = 1;
        [[serialize(0)]] int splitXCount_ = 1;
        [[serialize(0)]] int splitYCount_ = 1;
        [[serialize(0)]] int splitSizeX_  = 100;
        [[serialize(0)]] int splitSizeY_  = 100;

        std::string contentPath_;
        Guid guid_;
        
#pragma region Serialization Function
        public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<AssetBase>(this));
            archive(CEREAL_NVP(sourceType_));
            archive(cereal::make_nvp("sprites_", static_cast<uint32_t>(sprites_.size())));
            for (size_t i = 0; i < sprites_.size(); ++i)
            {
                archive(cereal::make_nvp("sprites_" + std::to_string(i), sprites_[i]));
            }
            archive(CEREAL_NVP(sprite_));
            archive(CEREAL_NVP(splitCount_));
            archive(CEREAL_NVP(splitXCount_));
            archive(CEREAL_NVP(splitYCount_));
            archive(CEREAL_NVP(splitSizeX_));
            archive(CEREAL_NVP(splitSizeY_));
            archive(CEREAL_NVP(contentPath_));
            archive(CEREAL_NVP(guid_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<AssetBase>(this));
            if (version >= 0) archive(CEREAL_NVP(sourceType_));
            if (version >= 0) archive(cereal::make_nvp("sprites_", static_cast<uint32_t>(sprites_.size())));
            for (size_t i = 0; i < sprites_.size(); ++i)
            {
                if (version >= 0) archive(cereal::make_nvp("sprites_" + std::to_string(i), sprites_[i]));
            }
            if (version >= 0) archive(CEREAL_NVP(sprite_));
            if (version >= 0) archive(CEREAL_NVP(splitCount_));
            if (version >= 0) archive(CEREAL_NVP(splitXCount_));
            if (version >= 0) archive(CEREAL_NVP(splitYCount_));
            if (version >= 0) archive(CEREAL_NVP(splitSizeX_));
            if (version >= 0) archive(CEREAL_NVP(splitSizeY_));
            if (version >= 0) archive(CEREAL_NVP(contentPath_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SpriteAnimationFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SpriteAnimationFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::SpriteAnimationFile);
#pragma endregion
REGISTER_ASSET(SpriteAnimationFile, SPRITE_ANIMATION_FILE_EXTENSION_LABEL)
REGISTER_CREATABLE_ASSET_EXTENSION("SpriteAnimation", SPRITE_ANIMATION_FILE_EXTENSION_LABEL)
