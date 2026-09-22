#include "PlayerAvatar_DefeatRequestQuest.h"

#include "../PlayerAvatar_TakeableQuestFactory.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::Request
{
    int DefeatRequestQuest::CurrentRecord(const Record::IRecordBook& records) const
    {
        return records.DefeatedCount(enemyKind_);
    }

    rxcpp::observable<int> DefeatRequestQuest::ObserveRecord(const Record::IRecordBook& records) const
    {
        const auto kind = enemyKind_;
        const auto* book = &records;
        return records.OnDefeat()
            .filter([kind](const Npc::Enemy::EnemyKind defeated) { return defeated == kind; })
            .map([kind, book](Npc::Enemy::EnemyKind) { return book->DefeatedCount(kind); });
    }

    void DefeatRequestQuest::DoDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("enemyKind_", enemyKind_, Npc::Enemy::ENEMY_KINDS, Npc::Enemy::ToString);
    }

    REGISTER_TAKEABLE_QUEST(DefeatRequestQuest)
}

CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Request::DefeatRequestQuest)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::Request::RequestQuestBase, GameCore::PlayerAvatar::Quest::Request::DefeatRequestQuest)
