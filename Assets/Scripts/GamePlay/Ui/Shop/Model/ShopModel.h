#pragma once
#include <memory>
#include <vector>

#include "../../../../../Data/Item/Data_ItemData.h"
#include "../../EventBoard/Model/BoardListCursor.h"

namespace GameCore::PlayerAvatar
{
    class ItemPouch;
    class Wallet;
}

namespace GamePlay::Ui
{
    /** @brief 選んだ品を今の個数で買えない理由 */
    enum class ShopRefusal
    {
        None,
        PouchFull,
        NotEnoughMoney,
    };

    struct ShopItemEntry final
    {
        std::shared_ptr<Asset::ItemData> item;
        int                              price = 0;
    };

    /**
     * @brief 店の品書きと、選んでいる品・個数。個数の上限はポーチの空きと所持金で決まる。
     * 財布とポーチは開いている間だけ借りる(アバターが無ければどちらも nullptr で、何も買えない)。
     */
    class ShopModel final
    {
    public:
        ShopModel(std::vector<ShopItemEntry> entries,
                  size_t visibleRowCount,
                  GameCore::PlayerAvatar::Wallet* wallet,
                  GameCore::PlayerAvatar::ItemPouch* pouch);

        [[nodiscard]] const std::vector<ShopItemEntry>& Entries() const { return entries_; }
        [[nodiscard]] BoardListCursor&       Cursor()       { return cursor_; }
        [[nodiscard]] const BoardListCursor& Cursor() const { return cursor_; }
        /** @brief 品が1つも無ければ nullptr */
        [[nodiscard]] const ShopItemEntry* Selected() const;

        [[nodiscard]] int Quantity() const { return quantity_; }
        [[nodiscard]] int TotalPrice() const;
        /** @brief ポーチの空きと所持金で買える数。1つも買えなければ 0 */
        [[nodiscard]] int MaxQuantity(const ShopItemEntry& entry) const;
        [[nodiscard]] ShopRefusal Refusal(const ShopItemEntry& entry) const;
        [[nodiscard]] int Owned(const ShopItemEntry& entry) const;
        [[nodiscard]] int Balance() const;

        /** @return 個数が変わったら true。1 と MaxQuantity の間で止まる */
        bool ChangeQuantity(int delta);
        /** @brief 品を選び直したときに 1 へ戻す */
        void ResetQuantity() { quantity_ = 1; }
        /** @brief 選んだ品を今の個数で買う @return 買えた数。買えなければ 0 */
        int Purchase();

    private:
        std::vector<ShopItemEntry> entries_;
        BoardListCursor cursor_;
        GameCore::PlayerAvatar::Wallet* wallet_;
        GameCore::PlayerAvatar::ItemPouch* pouch_;
        int quantity_ = 1;
    };
}
