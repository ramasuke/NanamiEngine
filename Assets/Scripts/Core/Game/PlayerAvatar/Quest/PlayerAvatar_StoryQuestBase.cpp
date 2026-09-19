#include "PlayerAvatar_StoryQuestBase.h"

#include <sstream>

#include "cereal/archives/json.hpp"

namespace GameCore::PlayerAvatar
{
    StoryQuestBase:: StoryQuestBase() = default;
    StoryQuestBase::~StoryQuestBase() = default;

    void StoryQuestBase::StartQuest(const Quest::QuestContext& context)
    {
        DoStartQuest(context);
    }

    void StoryQuestBase::OnDrawGui()
    {
        DrawRewardGui();
        DoDrawGui();
    }

    std::shared_ptr<StoryQuestBase> StoryQuestBase::Clone(const std::shared_ptr<StoryQuestBase>& source)
    {
        if (!source)
            return nullptr;

        // 掲示板のデータ(.meta)と同じ JSON を通すので、そこから読めたクエストなら必ず写せる
        std::stringstream ss;
        {
            cereal::JSONOutputArchive outputArchive(ss);
            outputArchive(cereal::make_nvp("quest", source));
        }

        std::shared_ptr<StoryQuestBase> copy;
        {
            cereal::JSONInputArchive inputArchive(ss);
            inputArchive(cereal::make_nvp("quest", copy));
        }
        return copy;
    }
}
