#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
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
        // NOTE: Setup かセーブからの読み込みを通ったか。通っていなければ初期所持を入れる
        [[nodiscard]] bool IsSetUp() const { return isSetUp_; }

        [[nodiscard]] const std::vector<Slot>& Slots        () const { return slots_; }
        [[nodiscard]] std::size_t              SelectedIndex() const { return selectedIndex_; }
        // NOTE: 選択中の枠。ポーチが空なら nullptr
        [[nodiscard]] const Slot*              Selected     () const;
        [[nodiscard]] bool                     CanUseSelected() const;
        // NOTE: 選択中の枠に残りがあり効果も持つならそのアイテム。使えなければ nullptr
        [[nodiscard]] std::shared_ptr<Asset::ItemData> SelectedUsableItem() const;
        // NOTE: 中身の入れ替わりを表す番号。表示側はこれが変わったときだけ作り直せばよい
        [[nodiscard]] std::uint32_t            Revision     () const { return revision_; }

        // NOTE: direction が正なら右隣、負なら左隣。端は反対側へ回り込む
        void Cycle(int direction);
        // NOTE: 選択中のアイテムの効果を target に掛けて 1 つ減らし、使ったアイテムを返す。使えなかったら nullptr
        std::shared_ptr<Asset::ItemData> UseSelected(Item::IItemEffectTarget& target, const std::shared_ptr<GameObject::IGameObject>& user);
        // NOTE: item の効果を target に掛けて 1 つ減らす。選択が途中で変わっても使えるよう item で指す。使えたら true
        bool Use(const Asset::ItemData& item, Item::IItemEffectTarget& target, const std::shared_ptr<GameObject::IGameObject>& user);

        // NOTE: 使うモーションに入る直前に使うアイテムを預けておく。TakePendingUse で取り出すと空に戻る
        void SetPendingUse(std::shared_ptr<Asset::ItemData> item) { pendingUse_ = std::move(item); }
        [[nodiscard]] std::shared_ptr<Asset::ItemData> TakePendingUse() { return std::exchange(pendingUse_, nullptr); }

        [[nodiscard]] int CountOf(const Asset::ItemData& item) const;
        [[nodiscard]] int ReceivableCount(const Asset::ItemData& item) const;
        // NOTE: 同じアイテムの枠に積む。枠が無ければ末尾に足す。実際に入った数を返す
        int Add(const std::shared_ptr<Asset::ItemData>& item, int count);

    private:
        [[nodiscard]] std::optional<std::size_t> FindSlotIndex(const Asset::ItemData& item) const;

        std::vector<Slot> slots_;
        std::shared_ptr<Asset::ItemData> pendingUse_;
        std::size_t       selectedIndex_ = 0;
        std::uint32_t     revision_ = 0;
        bool              isSetUp_ = false;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            // NOTE: ItemStack の FIELD は複製すると save の assert に掛かるので、その場で作って書く
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
