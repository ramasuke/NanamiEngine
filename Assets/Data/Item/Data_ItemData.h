#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/PlayerAvatar/Item/Effect/IItemEffect.h"

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;
}

namespace NanamiEngine::Module::Asset
{
    constexpr auto ITEM_DATA_EXTENSION_LABEL = ".itemData";

    // ポーチに入る消費アイテム1種類分の定義。所持数は持たず、誰が何個持っているかは ItemPouch 側
    class ItemData final : public ScriptableObject
    {
    public:
        explicit ItemData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&          DisplayName         () const { return displayName_;          }
        [[nodiscard]] std::shared_ptr<SpriteFile> IconSprite          () const { return iconSprite_.get();     }
        [[nodiscard]] std::shared_ptr<SoundFile>  UseSound            () const { return useSound_.get();       }
        [[nodiscard]] int                         MaxStack            () const { return maxStack_;             }
        [[nodiscard]] bool                        HasEffect           () const { return !effects_.empty();     }
        /** @brief 店の勘定書きなどに出す説明。1要素が1行 */
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        /** 地面に落ちたときの拾い物(ItemPickup 付き)。中身は生成側が渡すので、プレハブはこのアイテムを参照しない */
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> PickupPrefab() const { return pickupPrefab_.get(); }
        void ApplyEffects(GameCore::PlayerAvatar::Item::IItemEffectTarget& target) const;

    private:
        [[serialize(0)]] std::string       displayName_;
        [[serialize(0)]] FIELD(SpriteFile) iconSprite_;
        [[serialize(0)]] FIELD(SoundFile)  useSound_;
        [[serialize(0)]] int               maxStack_ = 10;
        [[serialize(1)]] std::vector<std::shared_ptr<GameCore::PlayerAvatar::Item::IItemEffect>> effects_;
        [[serialize(2)]] FIELD(PrefabGameObjectFile) pickupPrefab_;
        [[serialize(3)]] std::vector<std::string> descriptionLines_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(iconSprite_));
            archive(CEREAL_NVP(useSound_));
            archive(CEREAL_NVP(maxStack_));
            archive(CEREAL_NVP(effects_));
            archive(CEREAL_NVP(pickupPrefab_));
            archive(CEREAL_NVP(descriptionLines_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(iconSprite_));
            if (version >= 0) archive(CEREAL_NVP(useSound_));
            if (version >= 0) archive(CEREAL_NVP(maxStack_));
            if (version >= 1) archive(CEREAL_NVP(effects_));
            if (version >= 2) archive(CEREAL_NVP(pickupPrefab_));
            if (version >= 3) archive(CEREAL_NVP(descriptionLines_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(ItemData, ITEM_DATA_EXTENSION_LABEL, "Item")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::ItemData, 3);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::ItemData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::ItemData);
#pragma endregion
