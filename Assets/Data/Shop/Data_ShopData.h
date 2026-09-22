#pragma once
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "Data_ShopEntry.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto SHOP_DATA_EXTENSION_LABEL = ".shopData";

    /**
     * @brief 店の品揃え。品を足すときはこのアセットに1行足すだけで、UI もシーンも触らない。
     * 品物のアセットが消えている行は表示側で飛ばす。
     */
    class ShopData final : public ScriptableObject
    {
    public:
        explicit ShopData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&            Title  () const { return title_;   }
        [[nodiscard]] const std::vector<ShopEntry>& Entries() const { return entries_; }

    private:
        [[serialize(0)]] std::string            title_;
        [[serialize(0)]] std::vector<ShopEntry> entries_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(entries_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(entries_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(ShopData, SHOP_DATA_EXTENSION_LABEL, "Item")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::ShopData, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::ShopData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::ShopData);
#pragma endregion
