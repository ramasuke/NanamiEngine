#pragma once
#include <cstdint>

#include "../Item/Data_ItemData.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::Asset
{
    /** @brief ドロップ表の1行。壊れるたびに chance_ で独立に抽選し、当たれば count_ 個落とす */
    struct ItemDrop final
    {
        [[nodiscard]] std::shared_ptr<ItemData> Item  () const { return item_.get(); }
        [[nodiscard]] float                     Chance() const { return chance_; }
        [[nodiscard]] int                       Count () const { return count_; }

    private:
        [[serialize(0)]] FIELD(ItemData) item_;
        /** 0〜1 */
        [[serialize(0)]] float           chance_ = 0.1f;
        [[serialize(0)]] int             count_  = 1;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("item_", item_);
            LibCore::ImGuiHelper::OnDrawInputField("chance_", chance_);
            LibCore::ImGuiHelper::OnDrawInputField("count_", count_);
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(item_));
            archive(CEREAL_NVP(chance_));
            archive(CEREAL_NVP(count_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(item_));
            if (version >= 0) archive(CEREAL_NVP(chance_));
            if (version >= 0) archive(CEREAL_NVP(count_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::ItemDrop, 0)
