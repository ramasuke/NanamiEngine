#pragma once
#include <cstdint>

#include "../Item/Data_ItemData.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::Asset
{
    /** @brief 店の棚の1品。何を幾らで売るか */
    struct ShopEntry final
    {
        [[nodiscard]] std::shared_ptr<ItemData> Item () const { return item_.get(); }
        [[nodiscard]] int                       Price() const { return price_; }

    private:
        [[serialize(0)]] FIELD(ItemData) item_;
        [[serialize(0)]] int             price_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("item_", item_);
            LibCore::ImGuiHelper::OnDrawInputField("price_", price_);
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(item_));
            archive(CEREAL_NVP(price_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(item_));
            if (version >= 0) archive(CEREAL_NVP(price_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::ShopEntry, 0)
