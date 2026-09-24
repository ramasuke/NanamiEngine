#include "PlayerAvatar_ITakeableQuest.h"

#include <sstream>

#include "cereal/archives/json.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::PlayerAvatar::Quest
{
    std::shared_ptr<ITakeableQuest> ITakeableQuest::Clone() const
    {
        // NOTE: 多態のまま書き出すため shared_ptr で包む。所有はしないので何も消さない
        const std::shared_ptr<ITakeableQuest> source(const_cast<ITakeableQuest*>(this), [](ITakeableQuest*) {});

        // 掲示板のデータ(.meta)と同じ JSON を通すので、そこから読めたクエストなら必ず写せる
        std::stringstream ss;
        {
            cereal::JSONOutputArchive outputArchive(ss);
            outputArchive(cereal::make_nvp("quest", source));
        }

        std::shared_ptr<ITakeableQuest> copy;
        {
            cereal::JSONInputArchive inputArchive(ss);
            inputArchive(cereal::make_nvp("quest", copy));
        }
        return copy;
    }
}
