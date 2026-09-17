#pragma once
#include <cstdint>

#include "Data_ItemData.h"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::Asset
{
    /** @brief アイテム1種類とその所持数。ポーチの1枠にあたる */
    struct ItemStack final
    {
        [[nodiscard]] std::shared_ptr<ItemData> Item () const { return item_.get(); }
        [[nodiscard]] int                       Count() const { return count_; }
        void SetCount(const int count) { count_ = count; }

    private:
        [[serialize(0)]] FIELD(ItemData) item_;
        [[serialize(0)]] int             count_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("item_", item_);
            LibCore::ImGuiHelper::OnDrawInputField("count_", count_);
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(item_));
            archive(CEREAL_NVP(count_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(item_));
            if (version >= 0) archive(CEREAL_NVP(count_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::ItemStack, 0)
