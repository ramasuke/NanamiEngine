#pragma once
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto DECORATION_EXTENSION_LABEL = ".decoration";

    /**
     * @brief 島の飾り1つ。所持は DecorationCollection が guid で覚える
     */
    class DecorationData final : public ScriptableObject
    {
    public:
        explicit DecorationData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              Name            () const { return name_;             }
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        [[nodiscard]] std::shared_ptr<SpriteFile>     IconSprite      () const { return iconSprite_.get(); }

    private:
        [[serialize(0)]] std::string              name_;
        [[serialize(0)]] std::vector<std::string> descriptionLines_;
        [[serialize(0)]] FIELD(SpriteFile)        iconSprite_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(iconSprite_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(name_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(iconSprite_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::DecorationData, 0);
#pragma endregion
