#include "PlayerAvatar_CollectRequestQuest.h"

#include "../PlayerAvatar_TakeableQuestFactory.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::Request
{
    Guid CollectRequestQuest::ItemGuid() const
    {
        const auto item = item_.get();
        return item ? item->GetGuid() : Guid(std::string());
    }

    int CollectRequestQuest::CurrentRecord(const Record::IRecordBook& records) const
    {
        return records.AcquiredCount(ItemGuid());
    }

    NanamiEngine::R4::Observable<int> CollectRequestQuest::ObserveRecord(const Record::IRecordBook& records) const
    {
        const auto item = ItemGuid();
        const auto* book = &records;
        return records.OnAcquire()
            .Where([item](const Record::AcquiredRecord& acquired) { return acquired.item == item; })
            .Select([item, book](const Record::AcquiredRecord&) { return book->AcquiredCount(item); });
    }

    void CollectRequestQuest::DoDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("item_", item_);
    }

    REGISTER_TAKEABLE_QUEST(CollectRequestQuest)
}

CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Request::CollectRequestQuest)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::Request::RequestQuestBase, GameCore::PlayerAvatar::Quest::Request::CollectRequestQuest)
