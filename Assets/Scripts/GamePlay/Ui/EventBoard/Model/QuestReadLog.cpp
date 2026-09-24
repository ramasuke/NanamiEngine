#include "QuestReadLog.h"

#include <cereal/types/string.hpp>
#include <cereal/types/unordered_set.hpp>
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GamePlay::Ui
{
    QuestReadLog::QuestReadLog()
        : readGuids_(LocalPrefs::LoadOrDefault<std::unordered_set<std::string>>(
            QUEST_READ_LOG_SAVE_KEY,
            std::unordered_set<std::string>()))
    {
    }

    bool QuestReadLog::IsRead(const std::string& guid) const
    {
        return readGuids_.contains(guid);
    }

    bool QuestReadLog::MarkRead(const std::string& guid)
    {
        if (!readGuids_.insert(guid).second)
            return false;

        LocalPrefs::Save(QUEST_READ_LOG_SAVE_KEY, readGuids_);
        return true;
    }
}
