#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../../../../Data/Item/Data_ItemStack.h"

namespace GameCore::PlayerAvatar
{
    // 手持ちのアイテム。枠の並びは固定で、使い切っても枠は残す(モンハンのポーチと同じ見え方)。
    // セーブには乗せず、ステージに入るたび SwordManAvatarResource の初期所持から作り直す
    class ItemPouch final
    {
    public:
        struct Slot final
        {
            std::shared_ptr<Asset::ItemData> item;
            int                              count = 0;
        };

        void Setup(const std::vector<Asset::ItemStack>& initialItems);

        [[nodiscard]] const std::vector<Slot>& Slots        () const { return slots_; }
        [[nodiscard]] std::size_t              SelectedIndex() const { return selectedIndex_; }
        /** @brief 選択中の枠。ポーチが空なら nullptr */
        [[nodiscard]] const Slot*              Selected     () const;
        [[nodiscard]] bool                     CanUseSelected() const;
        /** @brief 中身の入れ替わりを1つの数で表す。UIはこれが変わったときだけ絵を作り直す */
        [[nodiscard]] std::uint32_t            Revision     () const { return revision_; }

        /** @param direction 正で右隣、負で左隣。端は反対側へ回り込む */
        void Cycle(int direction);
        /** @brief 選択中の枠を1つ減らす @return 減らせたか */
        bool ConsumeSelected();

    private:
        std::vector<Slot> slots_;
        std::size_t       selectedIndex_ = 0;
        std::uint32_t     revision_ = 0;
    };
}
