#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Data_ItemDrop.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/StatusParameter/Money/Money.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto DROP_TABLE_EXTENSION_LABEL = ".dropTable";

    /**
     * @brief 壊したときに落とすものの一覧。お金は毎回、アイテムは行ごとの確率で落とす。
     * 同じ種類の置き物はこのアセットを共有するので、シーン上の個々には何も書かない
     */
    class DropTable final : public ScriptableObject
    {
    public:
        explicit DropTable(const std::string& contentPath = "");

        [[nodiscard]] GameCore::StatusParameter::Money      TotalMoney       () const { return money_; }
        [[nodiscard]] int                                   MoneyPickupCount () const { return moneyPickupCount_; }
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> MoneyPickupPrefab() const { return moneyPickupPrefab_.get(); }
        [[nodiscard]] const std::vector<ItemDrop>&          Items            () const { return items_; }

    private:
        /** 合計額。moneyPickupCount_ 枚のコインに分けて散らす */
        [[serialize(0)]] GameCore::StatusParameter::Money money_;
        [[serialize(0)]] int                              moneyPickupCount_ = 1;
        [[serialize(0)]] FIELD(PrefabGameObjectFile)      moneyPickupPrefab_;
        [[serialize(0)]] std::vector<ItemDrop>            items_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(money_));
            archive(CEREAL_NVP(moneyPickupCount_));
            archive(CEREAL_NVP(moneyPickupPrefab_));
            archive(CEREAL_NVP(items_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(money_));
            if (version >= 0) archive(CEREAL_NVP(moneyPickupCount_));
            if (version >= 0) archive(CEREAL_NVP(moneyPickupPrefab_));
            if (version >= 0) archive(CEREAL_NVP(items_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(DropTable, DROP_TABLE_EXTENSION_LABEL, "Item")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::DropTable, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::DropTable);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::DropTable);
#pragma endregion
