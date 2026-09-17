#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto ITEM_DATA_EXTENSION_LABEL = ".itemData";

    /// アイテムを使ったときに起きること。1アイテムにつき1種類
    enum class ItemEffectType : std::uint8_t
    {
        None,
        HealHealth,
        RestoreStamina,
        EnhanceAttack,
    };

    constexpr std::array<ItemEffectType, 4> ITEM_EFFECT_TYPES = {
        ItemEffectType::None,
        ItemEffectType::HealHealth,
        ItemEffectType::RestoreStamina,
        ItemEffectType::EnhanceAttack,
    };

    constexpr std::string_view ToString(const ItemEffectType type)
    {
        switch (type)
        {
        case ItemEffectType::None:           return "None";
        case ItemEffectType::HealHealth:     return "HealHealth";
        case ItemEffectType::RestoreStamina: return "RestoreStamina";
        case ItemEffectType::EnhanceAttack:  return "EnhanceAttack";
        }
        return "None";
    }

    // ポーチに入る消費アイテム1種類分の定義。所持数は持たず、誰が何個持っているかは ItemPouch 側
    class ItemData final : public ScriptableObject
    {
    public:
        explicit ItemData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&          DisplayName         () const { return displayName_;          }
        [[nodiscard]] std::shared_ptr<SpriteFile> IconSprite          () const { return iconSprite_.get();     }
        [[nodiscard]] ItemEffectType              Effect              () const { return effect_;               }
        [[nodiscard]] float                       EffectAmount        () const { return effectAmount_;         }
        [[nodiscard]] float                       EffectDuration_secs () const { return effectDuration_secs_;  }
        [[nodiscard]] std::shared_ptr<SoundFile>  UseSound            () const { return useSound_.get();       }
        [[nodiscard]] int                         MaxStack            () const { return maxStack_;             }

    private:
        [[serialize(0)]] std::string      displayName_;
        [[serialize(0)]] FIELD(SpriteFile) iconSprite_;
        [[serialize(0)]] ItemEffectType   effect_ = ItemEffectType::None;
        [[serialize(0)]] float            effectAmount_ = 0.0f;
        [[serialize(0)]] float            effectDuration_secs_ = 0.0f;
        [[serialize(0)]] FIELD(SoundFile) useSound_;
        [[serialize(0)]] int              maxStack_ = 10;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(iconSprite_));
            archive(CEREAL_NVP(effect_));
            archive(CEREAL_NVP(effectAmount_));
            archive(CEREAL_NVP(effectDuration_secs_));
            archive(CEREAL_NVP(useSound_));
            archive(CEREAL_NVP(maxStack_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(iconSprite_));
            if (version >= 0) archive(CEREAL_NVP(effect_));
            if (version >= 0) archive(CEREAL_NVP(effectAmount_));
            if (version >= 0) archive(CEREAL_NVP(effectDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(useSound_));
            if (version >= 0) archive(CEREAL_NVP(maxStack_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(ItemData, ITEM_DATA_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::ItemData, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::ItemData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::ItemData);
#pragma endregion
