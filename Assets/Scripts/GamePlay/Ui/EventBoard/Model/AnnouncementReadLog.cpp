#include "AnnouncementReadLog.h"

#include <cereal/types/string.hpp>
#include <cereal/types/unordered_set.hpp>
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GamePlay::Ui
{
    AnnouncementReadLog::AnnouncementReadLog()
        : readGuids_(LocalPrefs::LoadOrDefault<std::unordered_set<std::string>>(
            ANNOUNCEMENT_READ_LOG_SAVE_KEY,
            std::unordered_set<std::string>()))
    {
    }

    bool AnnouncementReadLog::IsRead(const std::string& guid) const
    {
        return readGuids_.contains(guid);
    }

    bool AnnouncementReadLog::MarkRead(const std::string& guid)
    {
        if (!readGuids_.insert(guid).second)
            return false;

        LocalPrefs::Save(ANNOUNCEMENT_READ_LOG_SAVE_KEY, readGuids_);
        return true;
    }
}
