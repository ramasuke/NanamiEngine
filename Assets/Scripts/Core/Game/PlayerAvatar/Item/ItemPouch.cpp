#include "ItemPouch.h"

#include <algorithm>

namespace GameCore::PlayerAvatar
{
    void ItemPouch::Setup(const std::vector<Asset::ItemStack>& initialItems)
    {
        slots_.clear();
        for (const auto& stack : initialItems)
        {
            const auto item = stack.Item();
            if (!item)
                continue;
            slots_.push_back(Slot{ item, stack.Count() });
        }
        selectedIndex_ = 0;
        isSetUp_ = true;
        ++revision_;
    }

    const ItemPouch::Slot* ItemPouch::Selected() const
    {
        if (selectedIndex_ >= slots_.size())
            return nullptr;
        return &slots_[selectedIndex_];
    }

    bool ItemPouch::CanUseSelected() const
    {
        const auto selected = Selected();
        return selected != nullptr && selected->count > 0;
    }

    std::shared_ptr<Asset::ItemData> ItemPouch::SelectedUsableItem() const
    {
        if (!CanUseSelected())
            return nullptr;

        const auto& item = slots_[selectedIndex_].item;
        // 効果がまだ無いアイテムは使えない扱い
        if (!item || !item->HasEffect())
            return nullptr;
        return item;
    }

    void ItemPouch::Cycle(const int direction)
    {
        if (slots_.size() <= 1 || direction == 0)
            return;

        const auto size = static_cast<int>(slots_.size());
        const int stepped = static_cast<int>(selectedIndex_) + (direction > 0 ? 1 : -1);
        selectedIndex_ = static_cast<std::size_t>((stepped % size + size) % size);
        ++revision_;
    }

    std::shared_ptr<Asset::ItemData> ItemPouch::UseSelected(Item::IItemEffectTarget& target, const std::shared_ptr<GameObject::IGameObject>& user)
    {
        auto item = SelectedUsableItem();
        if (!item)
            return nullptr;

        item->ApplyEffects(target, user);
        --slots_[selectedIndex_].count;
        ++revision_;
        return item;
    }

    bool ItemPouch::Use(const Asset::ItemData& item, Item::IItemEffectTarget& target, const std::shared_ptr<GameObject::IGameObject>& user)
    {
        const auto index = FindSlotIndex(item);
        if (!index || slots_[*index].count <= 0 || !item.HasEffect())
            return false;

        item.ApplyEffects(target, user);
        --slots_[*index].count;
        ++revision_;
        return true;
    }

    std::optional<std::size_t> ItemPouch::FindSlotIndex(const Asset::ItemData& item) const
    {
        const auto it = std::ranges::find_if(slots_, [&](const Slot& slot) { return slot.item.get() == &item; });
        if (it == slots_.end())
            return std::nullopt;
        return static_cast<std::size_t>(it - slots_.begin());
    }

    int ItemPouch::CountOf(const Asset::ItemData& item) const
    {
        const auto index = FindSlotIndex(item);
        return index ? slots_[*index].count : 0;
    }

    int ItemPouch::ReceivableCount(const Asset::ItemData& item) const
    {
        return std::max(0, item.MaxStack() - CountOf(item));
    }

    int ItemPouch::Add(const std::shared_ptr<Asset::ItemData>& item, const int count)
    {
        if (!item || count <= 0)
            return 0;

        const int added = std::min(count, ReceivableCount(*item));
        if (added <= 0)
            return 0;

        const auto index = FindSlotIndex(*item);
        if (index)
            slots_[*index].count += added;
        else
            slots_.push_back(Slot{ item, added });

        ++revision_;
        return added;
    }
}
