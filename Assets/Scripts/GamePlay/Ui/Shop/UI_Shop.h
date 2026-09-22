#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Model/ShopModel.h"
#include "Receipt/Ui_ShopReceipt.h"
#include "Row/Ui_ShopRow.h"

namespace GamePlay::Ui
{
    /**
     * @brief 店の画面の見た目のまとめ役。左の黒板に品書きの行を並べ、右の勘定書きと財布の札に値を書く。
     * 行は自分の prefab を持ち、表示窓の分だけここで生やす。
     */
    class ShopUi final : public Component::ComponentBase
    {
    public:
        /** @brief 行を作る。2回目以降は何もしない */
        void BuildRows(size_t count);
        void SubscribeOnClickRow(const std::function<void(size_t)>& onClick) const;
        [[nodiscard]] size_t MaxVisibleRows() const { return static_cast<size_t>(maxVisibleRows_); }

        void SetTitle(const std::string& title) const;
        void SetMoney(int balance) const;
        /** @brief 表示窓に入っている品を行へ書き、選んだ品を勘定書きへ書く */
        void Bind(const ShopModel& model) const;
        void PlayPaidStamp() const;

    private:
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] float rowSpacing_px_ = 168.0f;
        [[serialize(0)]] int maxVisibleRows_ = 4;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreAboveMark_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreBelowMark_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) titleText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) restockText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) moneyText_;
        [[serialize(0)]] FIELD(ShopReceipt) receipt_;

        std::vector<std::weak_ptr<ShopRow>> rows_;
        // 「入荷待ち」は prefab で置いた行内の位置を保ったまま、空いた行へずらす
        float restockOffsetY_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(rowsRoot_));
            archive(CEREAL_NVP(rowSpacing_px_));
            archive(CEREAL_NVP(maxVisibleRows_));
            archive(CEREAL_NVP(moreAboveMark_));
            archive(CEREAL_NVP(moreBelowMark_));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(restockText_));
            archive(CEREAL_NVP(moneyText_));
            archive(CEREAL_NVP(receipt_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            if (version >= 0) archive(CEREAL_NVP(rowsRoot_));
            if (version >= 0) archive(CEREAL_NVP(rowSpacing_px_));
            if (version >= 0) archive(CEREAL_NVP(maxVisibleRows_));
            if (version >= 0) archive(CEREAL_NVP(moreAboveMark_));
            if (version >= 0) archive(CEREAL_NVP(moreBelowMark_));
            if (version >= 0) archive(CEREAL_NVP(titleText_));
            if (version >= 0) archive(CEREAL_NVP(restockText_));
            if (version >= 0) archive(CEREAL_NVP(moneyText_));
            if (version >= 0) archive(CEREAL_NVP(receipt_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopUi, 0)
