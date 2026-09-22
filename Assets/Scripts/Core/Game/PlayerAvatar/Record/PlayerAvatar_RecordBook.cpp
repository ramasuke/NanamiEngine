#include "PlayerAvatar_RecordBook.h"

#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"

namespace GameCore::PlayerAvatar::Record
{
    RecordBook::RecordBook()
        : data_(NanamiEngine::Module::LocalPrefs::LoadOrDefault<RecordBookData>(RECORD_BOOK_SAVE_KEY, RecordBookData()))
    {
    }

    void RecordBook::RecordDefeat(const Npc::Enemy::EnemyKind kind)
    {
        ++data_.defeated_[static_cast<int>(kind)];
        Save();
        onDefeat_.get_subscriber().on_next(kind);
    }

    void RecordBook::RecordAcquire(const Guid& item, const int count)
    {
        if (count <= 0 || item.Value().empty())
            return;

        data_.acquired_[item.Value()] += count;
        Save();
        onAcquire_.get_subscriber().on_next(AcquiredRecord{ item, count });
    }

    int RecordBook::DefeatedCount(const Npc::Enemy::EnemyKind kind) const
    {
        const auto found = data_.defeated_.find(static_cast<int>(kind));
        return found != data_.defeated_.end() ? found->second : 0;
    }

    int RecordBook::AcquiredCount(const Guid& item) const
    {
        const auto found = data_.acquired_.find(item.Value());
        return found != data_.acquired_.end() ? found->second : 0;
    }

    void RecordBook::Save() const
    {
        NanamiEngine::Module::LocalPrefs::Save(RECORD_BOOK_SAVE_KEY, data_);
    }

    void RecordBook::OnDrawGui() const
    {
        if (!ImGui::CollapsingHeader("RecordBook"))
            return;

        ImGui::Text("Defeated");
        for (const auto kind : Npc::Enemy::ENEMY_KINDS)
            ImGui::BulletText("%s: %d", Npc::Enemy::ToString(kind).data(), DefeatedCount(kind));

        ImGui::Text("Acquired");
        for (const auto& [item, count] : data_.acquired_)
            ImGui::BulletText("%s: %d", item.c_str(), count);
    }
}
