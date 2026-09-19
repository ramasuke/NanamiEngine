#include "ShopModel.h"

#include <algorithm>

#include "../../../../Core/Game/PlayerAvatar/Item/ItemPouch.h"
#include "../../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"

namespace GamePlay::Ui
{
    ShopModel::ShopModel(std::vector<ShopItemEntry> entries,
                         const size_t visibleRowCount,
                         GameCore::PlayerAvatar::Wallet* wallet,
                         GameCore::PlayerAvatar::ItemPouch* pouch)
        : entries_(std::move(entries))
        , cursor_(entries_.size(), visibleRowCount)
        , wallet_(wallet)
        , pouch_(pouch)
    {
    }

    const ShopItemEntry* ShopModel::Selected() const
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return nullptr;
        return &entries_[cursor_.SelectedIndex()];
    }

    int ShopModel::TotalPrice() const
    {
        const auto selected = Selected();
        return selected ? selected->price * quantity_ : 0;
    }

    int ShopModel::MaxQuantity(const ShopItemEntry& entry) const
    {
        if (!wallet_ || !pouch_ || !entry.item)
            return 0;

        const int room = pouch_->ReceivableCount(*entry.item);
        if (entry.price <= 0)
            return room;
        return std::min(room, wallet_->Balance().Value() / entry.price);
    }

    ShopRefusal ShopModel::Refusal(const ShopItemEntry& entry) const
    {
        if (!wallet_ || !pouch_ || !entry.item)
            return ShopRefusal::NotEnoughMoney;
        if (pouch_->ReceivableCount(*entry.item) <= 0)
            return ShopRefusal::PouchFull;
        if (!wallet_->CanAfford(GameCore::StatusParameter::Money(entry.price)))
            return ShopRefusal::NotEnoughMoney;
        return ShopRefusal::None;
    }

    int ShopModel::Owned(const ShopItemEntry& entry) const
    {
        return pouch_ && entry.item ? pouch_->CountOf(*entry.item) : 0;
    }

    int ShopModel::Balance() const
    {
        return wallet_ ? wallet_->Balance().Value() : 0;
    }

    bool ShopModel::ChangeQuantity(const int delta)
    {
        const auto selected = Selected();
        if (!selected)
            return false;

        const int next = std::clamp(quantity_ + delta, 1, std::max(1, MaxQuantity(*selected)));
        if (next == quantity_)
            return false;

        quantity_ = next;
        return true;
    }

    int ShopModel::Purchase()
    {
        const auto selected = Selected();
        if (!selected || Refusal(*selected) != ShopRefusal::None || quantity_ > MaxQuantity(*selected))
            return 0;

        if (!wallet_->TrySpend(GameCore::StatusParameter::Money(selected->price * quantity_)))
            return 0;

        const int bought = pouch_->Add(selected->item, quantity_);
        // 残りの上限が今の個数より小さくなったら合わせる
        quantity_ = std::clamp(quantity_, 1, std::max(1, MaxQuantity(*selected)));
        return bought;
    }
}
