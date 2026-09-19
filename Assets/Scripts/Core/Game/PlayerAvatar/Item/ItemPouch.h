#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../../../Data/Item/Data_ItemStack.h"

namespace GameCore::PlayerAvatar::Item
{
    class IItemEffectTarget;
}

namespace GameCore::PlayerAvatar
{
    class ItemPouch final
    {
    public:
        struct Slot final
        {
            std::shared_ptr<Asset::ItemData> item;
            int                              count = 0;
        };

        void Setup(const std::vector<Asset::ItemStack>& initialItems);
        /** @brief Setup かセーブからの読み込みを通ったか。通っていなければ初期所持を入れる */
        [[nodiscard]] bool IsSetUp() const { return isSetUp_; }

        [[nodiscard]] const std::vector<Slot>& Slots        () const { return slots_; }
        [[nodiscard]] std::size_t              SelectedIndex() const { return selectedIndex_; }
        /** @brief 選択中の枠。ポーチが空なら nullptr */
        [[nodiscard]] const Slot*              Selected     () const;
        [[nodiscard]] bool                     CanUseSelected() const;
        /** @brief 中身の入れ替わりを1つの数で表す。UIはこれが変わったときだけ絵を作り直す */
        [[nodiscard]] std::uint32_t            Revision     () const { return revision_; }

        /** @param direction 正で右隣、負で左隣。端は反対側へ回り込む */
        void Cycle(int direction);
        /** @brief 選択中のアイテムの効果を target に掛けて1つ減らす @return 使ったアイテム。使えなかったら nullptr */
        std::shared_ptr<Asset::ItemData> UseSelected(Item::IItemEffectTarget& target);

        [[nodiscard]] int CountOf(const Asset::ItemData& item) const;
        [[nodiscard]] int ReceivableCount(const Asset::ItemData& item) const;
        /** @brief 同じアイテムの枠に積む。枠が無ければ末尾に足す @return 実際に入った数 */
        int Add(const std::shared_ptr<Asset::ItemData>& item, int count);

    private:
        /** @return 見つからなければ slots_.size() */
        [[nodiscard]] std::size_t FindSlotIndex(const Asset::ItemData& item) const;

        std::vector<Slot> slots_;
        std::size_t       selectedIndex_ = 0;
        std::uint32_t     revision_ = 0;
        bool              isSetUp_ = false;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            // ItemStack の FIELD は複製すると save の assert に掛かるので、その場で作って書く
            std::vector<Asset::ItemStack> stacks;
            stacks.reserve(slots_.size());
            for (const auto& slot : slots_)
                stacks.emplace_back(slot.item, slot.count);
            archive(cereal::make_nvp("stacks_", stacks));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            std::vector<Asset::ItemStack> stacks;
            if (version >= 0) archive(cereal::make_nvp("stacks_", stacks));
            for (auto& stack : stacks)
                stack.ResolveItem();
            Setup(stacks);
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::ItemPouch, 0)
#pragma endregion
