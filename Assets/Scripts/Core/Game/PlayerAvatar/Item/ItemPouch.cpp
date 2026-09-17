#include "ItemPouch.h"

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

    bool ItemPouch::ConsumeSelected()
    {
        if (!CanUseSelected())
            return false;

        --slots_[selectedIndex_].count;
        ++revision_;
        return true;
    }
}
