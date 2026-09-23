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
        if (!CanUseSelected())
            return nullptr;

        auto& slot = slots_[selectedIndex_];
        // 効果がまだ無いアイテムは減らさない
        if (!slot.item || !slot.item->HasEffect())
            return nullptr;

        slot.item->ApplyEffects(target, user);
        --slot.count;
        ++revision_;
        return slot.item;
    }

    bool ItemPouch::Use(const Asset::ItemData& item, Item::IItemEffectTarget& target, const std::shared_ptr<GameObject::IGameObject>& user)
    {
        const std::size_t index = FindSlotIndex(item);
        if (index >= slots_.size() || slots_[index].count <= 0 || !item.HasEffect())
            return false;

        item.ApplyEffects(target, user);
        --slots_[index].count;
        ++revision_;
        return true;
    }

    std::size_t ItemPouch::FindSlotIndex(const Asset::ItemData& item) const
    {
        const auto it = std::ranges::find_if(slots_, [&](const Slot& slot) { return slot.item.get() == &item; });
        return static_cast<std::size_t>(it - slots_.begin());
    }

    int ItemPouch::CountOf(const Asset::ItemData& item) const
    {
        const std::size_t index = FindSlotIndex(item);
        return index < slots_.size() ? slots_[index].count : 0;
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

        const std::size_t index = FindSlotIndex(*item);
        if (index < slots_.size())
            slots_[index].count += added;
        else
            slots_.push_back(Slot{ item, added });

        ++revision_;
        return added;
    }
}
