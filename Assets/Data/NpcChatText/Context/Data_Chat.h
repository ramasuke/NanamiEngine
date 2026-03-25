#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/Font/Ttf/TtfFontFile.h"
#include "../../../../Engine/Module/Color/Color32.h"
#include "../../../Engine/Module/Namespace/EngineNamespace.h"

namespace GamePlay::Data
{
    class Chat final
    {
    public:
        [[nodiscard]] std::shared_ptr<Asset::TtfFontFile> Font     () const { return font_.get(); }
        [[nodiscard]] const std::string&               Text     () const { return text_ ;      }
        [[nodiscard]] const Color32&                   TextColor() const { return textColor_;  }
        
    private:
        [[serialize(0)]] FIELD(Asset::TtfFontFile) font_;
        [[serialize(0)]] std::string text_;
        [[serialize(0)]] Color32 textColor_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(font_));
            archive(CEREAL_NVP(text_));
            archive(CEREAL_NVP(textColor_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if(version >= 0) archive(CEREAL_NVP(font_));
            if(version >= 0) archive(CEREAL_NVP(text_));
            if(version >= 0) archive(CEREAL_NVP(textColor_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Data::Chat, 0)