#include "CharacterSelectModel.h"

#include <algorithm>
#include <utility>

namespace GamePlay::Ui
{
    CharacterSelectModel::CharacterSelectModel(std::vector<std::shared_ptr<Asset::CharacterData>> characters)
        : characters_(std::move(characters))
    {
    }

    std::shared_ptr<Asset::CharacterData> CharacterSelectModel::Selected() const
    {
        if (selectedIndex_ >= characters_.size())
            return nullptr;

        return characters_[selectedIndex_];
    }

    bool CharacterSelectModel::CanConfirm() const
    {
        const auto selected = Selected();
        return selected && selected->IsUnlocked();
    }

    void CharacterSelectModel::Select(const size_t index)
    {
        if (index >= characters_.size() || index == selectedIndex_)
            return;

        selectedIndex_ = index;
        onSelectionChanged_.get_subscriber().on_next(selectedIndex_);
    }

    void CharacterSelectModel::MoveSelection(const int delta)
    {
        if (characters_.empty())
            return;

        const int last = static_cast<int>(characters_.size()) - 1;
        const int next = std::clamp(static_cast<int>(selectedIndex_) + delta, 0, last);
        Select(static_cast<size_t>(next));
    }
}
