#include "ItemUseAction.h"

#include "ItemPouch.h"
#include "../../../../../Data/Item/Data_ItemData.h"
#include "../../../../GamePlay/Item/GamePlay_ItemUseCue.h"

namespace GameCore::PlayerAvatar::Item
{
    void ItemUseAction::Begin(ItemPouch& pouch)
    {
        item_    = pouch.TakePendingUse();
        hasUsed_ = false;
    }

    void ItemUseAction::Update(const float during_secs, ItemPouch& pouch, IItemEffectTarget& target,
                               const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& user)
    {
        if (!item_ || hasUsed_ || during_secs < item_->UseEffectTime_secs())
            return;

        hasUsed_ = true;
        if (pouch.Use(*item_, target, user))
            GamePlay::Item::PlayItemUseCue(*item_, user);
    }

    void ItemUseAction::End()
    {
        item_.reset();
        hasUsed_ = false;
    }

    bool ItemUseAction::IsFinished(const float during_secs) const
    {
        return !item_ || during_secs >= item_->UseTotalDuration_secs();
    }
}
